# RHI D3D12 Queue Clock Calibration Foundation v1 (2026-05-06)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `rhi-d3d12-queue-clock-calibration-foundation-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Add a backend-private D3D12 queue clock calibration foundation for graphics and compute queues so future measured async
compute morph diagnostics can correlate queue timestamp intervals with CPU `QueryPerformanceCounter` ticks without
exposing native handles or package-visible performance claims.

## Architecture

Keep `ID3D12CommandQueue::GetClockCalibration`, raw GPU timestamp samples, CPU QPC samples, native queues, and any
future cross-queue timing math inside `mirakana_rhi_d3d12` backend diagnostics. Expose only D3D12-backend-specific first-party
calibration rows to the D3D12 test surface; do not add backend-neutral RHI, runtime-host, generated package, gameplay,
or editor APIs in this slice.

## Tech Stack

C++23, `mirakana_rhi_d3d12`, D3D12 `GetClockCalibration`, per-queue timestamp frequency, focused D3D12 tests, and existing
`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` repository validation.

---

## Official References

- [Microsoft Learn: Timing (Direct3D 12 Graphics)](https://learn.microsoft.com/en-us/windows/win32/direct3d12/timing)
  documents `ID3D12CommandQueue::GetClockCalibration` for correlating GPU timestamps with CPU
  `QueryPerformanceCounter` samples and notes that unsupported timestamp queues return `E_FAIL`.
- [Microsoft Learn: Multi-engine synchronization](https://learn.microsoft.com/en-us/windows/win32/direct3d12/user-mode-heap-synchronization)
  documents independent graphics/compute/copy queues and the need for explicit queue synchronization before making
  cross-queue ordering claims.

## Context

- RHI D3D12 Queue Timestamp Measurement Foundation v1 added D3D12 graphics/compute queue interval measurement with
  timestamp query heaps, `ResolveQueryData`, queue-local fence waits, and floating-point seconds conversion.
- The next measured-overlap diagnostic needs a correlation anchor before comparing work submitted to different D3D12
  queues. Microsoft documents `GetClockCalibration` as the official way to correlate a queue GPU timestamp counter
  with the CPU performance counter.
- Copy queue calibration remains out of scope because copy queue timestamp support is optional and not needed for the
  compute-morph graphics/compute path.

## Constraints

- Do not expose `ID3D12CommandQueue`, `ID3D12QueryHeap`, fences, events, command lists, native handles, or clock
  calibration data through gameplay, runtime-host, generated package, editor, or backend-neutral RHI APIs.
- Do not claim async overlap, speedup, package-visible performance readiness, Vulkan/Metal parity, frame graph
  scheduling, or generated package readiness in this slice.
- Do not add deterministic tests that require graphics and compute queues to overlap; this slice only proves calibration
  samples are available for supported queues.
- Keep the implementation D3D12-primary and backend-private. NullRHI, Vulkan, and Metal should not change unless a
  compile break requires it.

## File Map

- `engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp`: Add backend-specific clock calibration diagnostic types and
  a `DeviceContext` inspection method.
- `engine/rhi/d3d12/src/d3d12_backend.cpp`: Implement graphics/compute queue calibration through `GetClockCalibration`
  and queue-local timestamp frequency checks.
- `tests/unit/d3d12_rhi_tests.cpp`: Add RED/GREEN focused tests for graphics and compute queue clock calibration support.
- `docs/current-capabilities.md`, `docs/roadmap.md`, the plan registry, `engine/agent/manifest.json`,
  `.agents/skills/*`, `.claude/skills/*`, `.codex/agents/*`, `.claude/agents/*`, and `tools/check-ai-integration.ps1`:
  Keep the active slice and capability claim narrow and machine-readable.

## Done When

- A RED D3D12 test fails first because there is no backend-private queue clock calibration API for graphics and compute
  queues.
- D3D12 backend tests can request graphics and compute queue calibration samples independently.
- Calibration diagnostics include queue kind, status, timestamp frequency, GPU timestamp sample, CPU QPC sample, and a
  clear diagnostic string without exposing native handles or package-visible timing counters.
- The implementation uses `ID3D12CommandQueue::GetClockCalibration` only after queue timestamp support/frequency has
  been confirmed for that queue.
- The tests assert only support, finite samples, and status strings, not cross-queue overlap or speedup.
- Docs, manifest, plan registry, static checks, skills/subagents, and validation evidence describe this as a D3D12
  clock calibration foundation only.
- Focused D3D12 tests plus `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`,
  `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -MaxFiles 1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass, or
  concrete host/tool blockers are recorded.

## Tasks

- [x] Inspect the timestamp measurement API and existing D3D12 queue creation/frequency code.
- [x] Add RED D3D12 backend tests for graphics/compute clock calibration.
- [x] Add backend-specific first-party calibration diagnostic types to `d3d12_backend.hpp`.
- [x] Implement graphics/compute `GetClockCalibration` sampling in `d3d12_backend.cpp`.
- [x] Keep overlap classification conservative: no cross-queue overlap assertion and no generated package or runtime-host
  performance counters.
- [x] Update docs, manifest, plan registry, static checks, skills/subagents, and validation evidence.

## Validation Evidence

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` failed in `tests/unit/d3d12_rhi_tests.cpp` because
  `DeviceContext::calibrate_queue_clock` and `mirakana::rhi::d3d12::QueueClockCalibrationStatus` did not exist.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` passed after adding `QueueClockCalibrationStatus`, `QueueClockCalibration`, and
  `DeviceContext::calibrate_queue_clock`.
- GREEN: `ctest --preset dev -R mirakana_d3d12_rhi_tests --output-on-failure` passed, including graphics and compute queue
  clock calibration tests.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed; the new calibration surface is D3D12-backend-specific and does not add
  native D3D12/Win32 symbols to backend-neutral public headers.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after adding static assertions for the active calibration plan, D3D12 backend
  diagnostic types, `GetClockCalibration`, and focused tests.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -MaxFiles 1` passed.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed. Full CTest completed 29/29, including `mirakana_d3d12_rhi_tests`; Metal shader tools
  and Apple packaging remain diagnostic-only host blockers on this Windows machine.
