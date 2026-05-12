# Crash Telemetry Trace Ops v1 Implementation Plan (2026-05-06)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Turn the existing `mirakana_core` diagnostics/profile/Trace Event JSON foundation into an explicit operations handoff contract for summaries, trace artifacts, crash-dump review gates, and unsupported telemetry upload claims.

**Status:** Completed on 2026-05-06

**Architecture:** Keep the contract standard-library-only inside `mirakana_core`. Build a value-type diagnostics ops plan from a `DiagnosticCapture` plus host/backend availability flags. Ready artifacts must point at first-party helpers such as `summarize_diagnostics` and `export_diagnostics_trace_json`; crash dump review must be host-gated by official debugger availability; telemetry upload and crash-report backend claims must remain unsupported unless an explicit backend is configured by the caller.

**Tech Stack:** C++23 `mirakana_core`, unit tests in `mirakana_core_tests`, docs/manifest/static AI guidance sync.

---

## Context

- `mirakana_core` already provides dependency-free diagnostics events, counters, CPU profile samples, bounded recording, summaries, and Chrome Trace Event JSON export.
- The master plan still lists `crash-telemetry-trace-ops-v1` as a remaining platform/release/operations split.
- Windows native crash/dump review uses official Debugging Tools for Windows and Microsoft public symbols as host diagnostics only; these tools are not default repository dependencies.
- No crash-report backend, telemetry upload backend, symbol server publication, signing, upload, notarization, Android matrix, or Apple-host readiness should be implied by this slice.

## Constraints

- Keep `engine/core` independent from OS APIs, platform handles, renderer/RHI, editor, SDL3, Dear ImGui, and network/upload dependencies.
- Do not add a JSON parser, external telemetry SDK, crash reporter, native dump writer, symbol uploader, or backend-specific code.
- Do not expose native handles or commands through the public C++ contract.
- Tests first: add failing `mirakana_core_tests` coverage before implementation.
- End with focused core validation, API boundary/static checks, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

## Tasks

### Task 1: RED Diagnostics Ops Tests

**Files:**
- Modify: `tests/unit/core_tests.cpp`

- [x] Add coverage for a diagnostics ops plan built from events, counters, and profiles.
- [x] Require a ready summary artifact with counts from `summarize_diagnostics`.
- [x] Require a ready `trace_event_json` artifact that names `export_diagnostics_trace_json`, the Chrome Trace Event JSON format, and the capture counts.
- [x] Require crash dump review to be host-gated when Debugging Tools for Windows are unavailable, with a blocker that names `Debugging Tools for Windows`.
- [x] Require telemetry upload to be unsupported when no telemetry backend is configured.
- [x] Run `cmake --build --preset dev --target mirakana_core_tests` and confirm RED because the new public API is missing.

### Task 2: Implement mirakana_core Ops Contract

**Files:**
- Modify: `engine/core/include/mirakana/core/diagnostics.hpp`
- Modify: `engine/core/src/diagnostics.cpp`

- [x] Add value types for diagnostics ops artifact kind, status, host status, plan options, artifact rows, and plan results.
- [x] Add `build_diagnostics_ops_plan(const DiagnosticCapture&, const DiagnosticsOpsPlanOptions&)`.
- [x] Keep statuses deterministic: summary and trace artifacts are ready, crash dump review is ready only when debugger tools are available and host-gated when debugger tools are unavailable, and telemetry upload is unsupported without a configured telemetry backend.
- [x] Keep artifact rows free of shell commands and native handles.

### Task 3: Docs, Manifest, And Static Checks

**Files:**
- Modify: `docs/current-capabilities.md`
- Modify: `docs/testing.md`
- Modify: `docs/workflows.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`

- [x] Document the new `DiagnosticsOpsPlan` handoff contract and its boundaries.
- [x] Update the master plan so `crash-telemetry-trace-ops-v1` is no longer a remaining split, while Android and Apple host lanes remain separate.
- [x] Update manifest guidance so generated games and host adapters use the first-party ops handoff instead of inventing local profiling/telemetry formats.
- [x] Add static checks so the ops contract and boundary claims remain discoverable.

### Task 4: Validation

- [x] Run `cmake --build --preset dev --target mirakana_core_tests`.
- [x] Run `ctest --preset dev -R mirakana_core_tests --output-on-failure`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record validation evidence here and return `currentActivePlan` to the master plan.

## Done When

- `mirakana_core` exposes a deterministic diagnostics ops handoff plan for summaries, Trace Event JSON, crash dump review gates, and telemetry upload support.
- Tests prove ready summary/trace artifacts, host-gated crash review when debugger tools are unavailable, and unsupported telemetry upload without a backend.
- Docs, manifest, static checks, and the plan registry describe the operations boundary honestly.
- No crash-report backend, telemetry upload SDK, symbol publication, signing, upload, notarization, Android matrix, Apple-host readiness, native handle, or shell-command readiness claim is added.
- The completed slice is explicitly without adding native dump writing, telemetry SDK/upload execution, symbol publication, trace import tooling, flame graph UI, or native handle exposure.

## Validation Results

- RED: `cmake --build --preset dev --target mirakana_core_tests` failed before implementation because `mirakana::DiagnosticsOpsPlanOptions`, `mirakana::DiagnosticsOpsArtifactKind`, `mirakana::DiagnosticsOpsArtifactStatus`, `mirakana::DiagnosticsOpsArtifact`, and `mirakana::build_diagnostics_ops_plan` did not exist.
- PASS: `cmake --build --preset dev --target mirakana_core_tests`.
- PASS: `ctest --preset dev -R mirakana_core_tests --output-on-failure`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- PASS: `Get-Content -Raw -Path engine/agent/manifest.json | ConvertFrom-Json`.
- FIXED: First `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` attempt failed because `docs/workflows.md` documented `mirakana::build_diagnostics_ops_plan` without the literal `DiagnosticsOpsPlan` marker required by the new static check.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` completed license/config/json/recipe/SDK/package/AI/dependency/toolchain/format/tidy/build/CTest validation; shader and Apple checks reported the existing diagnostic-only Metal/Apple host blockers.
- `engine/agent/manifest.json` keeps `currentActivePlan` on the master plan and returns `recommendedNextPlan.id` to `next-production-gap-selection`.
