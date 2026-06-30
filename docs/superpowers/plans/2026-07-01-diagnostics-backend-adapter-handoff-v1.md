# Diagnostics Backend Adapter Handoff v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `diagnostics-backend-adapter-handoff-v1`
**Status:** Completed.
**Date:** 2026-07-01

**Goal:** Make the existing dependency-free diagnostics operations handoff explicit enough for reviewed crash-review and telemetry backend adapters without adding native dump writing, symbol publication, telemetry SDKs, upload execution, network dependencies, OS handles, or compatibility shims.

**Context:** `Crash Telemetry Trace Ops v1` already created `DiagnosticsOpsPlan`, but its crash and telemetry readiness used bool-only host status flags. That allowed a caller to mark telemetry ready without naming the backend, OpenTelemetry resource `service.name`, payload contract review, redaction review, or operator handoff evidence.

**Constraints:**
- Keep `MK_core` standard-library-only and value-only.
- Replace the bool-only host status contract with clean adapter descriptors; do not keep compatibility aliases or migration shims.
- Follow the OpenTelemetry trace data model as a caller-owned payload contract: resource attributes including `service.name`, `resourceSpans`, span identifiers, events, links, status, and Unix nanosecond timing.
- Treat Debugging Tools for Windows as host-owned crash review evidence only. Do not write native dumps, execute debuggers, publish symbols, or expose native handles.
- Keep Unity, Unreal Engine, and Godot out of this slice; no external engine code, assets, schemas, samples, UI expression, trademarks, compatibility, parity, or equivalence claims are introduced.

**Done When:**
- `DiagnosticsOpsPlanOptions` accepts explicit crash-review and telemetry backend descriptors.
- `build_diagnostics_ops_plan` reports telemetry `unsupported` when no backend descriptor exists, `host_gated` when a partial descriptor lacks OpenTelemetry/service/redaction/operator proof, and `ready` only when the descriptor is complete.
- Editor profiler telemetry handoff rows expose backend id, service name, schema id, and payload contract without uploading telemetry.
- Docs, plan registry, manifest fragments, composed manifest, and static checks agree on the active slice and non-claims.
- Focused tests, agent-surface checks, public API checks, formatting, tidy, and full validation pass or record a concrete host/tool blocker.

## Implementation Tasks

- [x] Add failing core/editor tests for descriptor-backed crash and telemetry handoff readiness.
- [x] Replace bool-only diagnostics operations host status with `DiagnosticsCrashReviewAdapterDesc`, `DiagnosticsTelemetryBackendDesc`, and `DiagnosticsOpsAdapterDesc`.
- [x] Add fail-closed readiness validation for Debugging Tools/symbol/dump/operator evidence and OpenTelemetry backend/service/redaction/operator evidence.
- [x] Expose backend id, service name, schema id, and payload contract through editor profiler telemetry rows.
- [x] Synchronize docs, roadmap, plan registry, backlog, manifest fragments, and static checks.
- [x] Run validation gates and record final evidence.

## Validation Evidence

| Gate | Evidence |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1` | Passed before focused build. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev` | Passed and generated `out/build/dev`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests MK_editor_core_tests` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_core_tests|MK_editor_core_tests"` | Passed: 2/2 tests. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed after composing `engine/agent/manifest.json`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/core/src/diagnostics.cpp,tests/unit/core_tests.cpp,editor/core/src/profiler.cpp,tests/unit/editor_core_tests.cpp` | Passed for the changed C++ sources/tests. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed at the final closeout gate. |
