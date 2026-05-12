# RHI D3D12 Calibrated Queue Timing Diagnostics v1 (2026-05-06)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `rhi-d3d12-calibrated-queue-timing-diagnostics-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Build a backend-private D3D12 calibrated queue timing diagnostic that combines timestamp query intervals with
`GetClockCalibration` samples so future compute/graphics async-overlap evidence can compare graphics and compute queue
work on one CPU-QPC timeline without exposing native handles or package-visible performance claims.

## Architecture

Keep query heaps, readback resources, raw GPU timestamp samples, CPU QPC samples, native queues/fences, and calibrated
time math inside `mirakana_rhi_d3d12`. Expose only D3D12-backend-specific first-party diagnostic rows to D3D12 tests; do not
add backend-neutral RHI, runtime-host, generated package, gameplay, or editor APIs in this slice.

## Tech Stack

C++23, `mirakana_rhi_d3d12`, D3D12 timestamp queries, `ID3D12CommandQueue::GetClockCalibration`, queue-local timestamp
frequency, focused D3D12 tests, and existing `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` repository validation.

---

## Official References

- [Microsoft Learn: Timing (Direct3D 12 Graphics)](https://learn.microsoft.com/en-us/windows/win32/direct3d12/timing)
  documents timestamp queries, per-queue timestamp frequency, floating-point seconds conversion, and
  `GetClockCalibration` for correlating GPU timestamps with CPU `QueryPerformanceCounter` samples.
- [Microsoft Learn: Multi-engine synchronization](https://learn.microsoft.com/en-us/windows/win32/direct3d12/user-mode-heap-synchronization)
  documents independent graphics/compute/copy queues and explicit queue synchronization requirements.

## Context

- RHI D3D12 Queue Timestamp Measurement Foundation v1 completed backend-private graphics/compute queue interval
  measurement using D3D12 timestamp query heaps, `ResolveQueryData`, readback mapping, and queue-local fence waits.
- RHI D3D12 Queue Clock Calibration Foundation v1 completed graphics/compute queue `GetClockCalibration` diagnostics.
- The next useful evidence step is a single backend diagnostic that returns calibrated interval rows. Actual async
  overlap/speedup claims remain a later opt-in decision because host scheduling may serialize valid queue work.

## Constraints

- Do not expose `ID3D12QueryHeap`, `ID3D12CommandQueue`, native fences, command lists, descriptor handles, events, raw
  GPU timestamps, CPU QPC samples, or calibrated timing rows through gameplay, runtime-host, generated package, editor,
  or backend-neutral RHI APIs.
- Do not claim async overlap, speedup, package-visible performance readiness, Vulkan/Metal parity, frame graph
  scheduling, generated package readiness, or broad renderer quality in this slice.
- Do not add deterministic tests that require graphics and compute queue intervals to overlap.
- Keep copy queue out of scope unless a later plan validates optional copy timestamp support.

## File Map

- `engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp`: Add backend-specific calibrated timing diagnostic types and
  a `DeviceContext` method.
- `engine/rhi/d3d12/src/d3d12_backend.cpp`: Implement calibrated interval capture by combining timestamp measurement,
  queue frequency, and before/after `GetClockCalibration` samples.
- `tests/unit/d3d12_rhi_tests.cpp`: Add RED/GREEN focused tests for graphics and compute calibrated timing diagnostics.
- Docs, manifest, plan registry, static checks, skills/subagents, and `tools/check-ai-integration.ps1`: Keep the active
  claim narrow and machine-readable.

## Done When

- A RED D3D12 test fails first because there is no calibrated queue timing diagnostic API.
- D3D12 backend tests can capture graphics and compute calibrated interval rows independently.
- Diagnostics include queue kind, status, frequency, begin/end GPU ticks, elapsed seconds, before/after calibration CPU
  QPC samples, and a clear diagnostic string.
- The implementation reuses the backend-private timestamp measurement and queue clock calibration foundations without
  leaking native handles or raw timing data outside the D3D12 backend test surface.
- Tests assert availability, ordering, finite conversion, and conservative diagnostics only, not overlap or speedup.
- Focused D3D12 tests plus `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`,
  `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -MaxFiles 1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass, or
  concrete host/tool blockers are recorded.

## Tasks

- [x] Inspect the timestamp interval and clock calibration APIs added by the previous two slices.
- [x] Add RED D3D12 backend tests for graphics/compute calibrated queue timing diagnostics.
- [x] Add backend-specific first-party calibrated timing diagnostic types to `d3d12_backend.hpp`.
- [x] Implement calibrated interval capture in `d3d12_backend.cpp`.
- [x] Keep overlap classification conservative: no deterministic cross-queue overlap assertion and no generated package
  or runtime-host performance counters.
- [x] Update docs, manifest, plan registry, static checks, skills/subagents, and validation evidence.

## Validation Evidence

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` failed in `tests/unit/d3d12_rhi_tests.cpp` because
  `DeviceContext::measure_calibrated_queue_timing` and
  `mirakana::rhi::d3d12::QueueCalibratedTimingStatus` did not exist.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` passed after adding `QueueCalibratedTimingStatus`, `QueueCalibratedTiming`, QPC conversion,
  and `DeviceContext::measure_calibrated_queue_timing`.
- GREEN: `ctest --preset dev -R mirakana_d3d12_rhi_tests --output-on-failure` passed, including graphics and compute
  calibrated queue timing diagnostics.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed after clang-format normalized the QPC delta ternary expression.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed after the backend-specific header change.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed with the updated active-plan/static integration assertions.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -MaxFiles 1` passed
  (`30977 warnings generated.` from the existing compile database smoke, exit 0).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed: 29/29 CTest tests passed, `validate: ok`; Metal shader tools and Apple packaging
  remained diagnostic-only host blockers on Windows.
