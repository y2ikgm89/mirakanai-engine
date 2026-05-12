# RHI D3D12 Queue Timestamp Measurement Foundation v1 (2026-05-06)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `rhi-d3d12-queue-timestamp-measurement-foundation-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Add a backend-private D3D12 queue timestamp measurement foundation for graphics and compute queues so future async
compute morph overlap evidence can use real queue timing without exposing native D3D12 handles, query heaps, or raw GPU
timestamp values to gameplay or generated package APIs.

## Architecture

Keep timestamp query heaps, readback resources, command lists, clock calibration, and queue frequency details inside
`mirakana_rhi_d3d12`. Expose only backend-specific first-party diagnostics in the D3D12 backend test surface; do not add
runtime-host or package-visible performance counters in this slice. Treat measured overlap as a later opt-in claim that
must handle host variance, not as a deterministic test expectation.

## Tech Stack

C++23, `mirakana_rhi_d3d12`, D3D12 timestamp queries, D3D12 command queue timestamp frequency/calibration, focused D3D12
tests, and existing `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` repository validation.

---

## Official References

- [Microsoft Learn: Timing (Direct3D 12 Graphics)](https://learn.microsoft.com/en-us/windows/win32/direct3d12/timing)
  documents per-command-queue timestamp frequency, D3D12 timestamp query heap types, and the requirement to use
  floating-point arithmetic when converting ticks to seconds.
- [Microsoft Learn: Multi-engine synchronization](https://learn.microsoft.com/en-us/windows/win32/direct3d12/user-mode-heap-synchronization)
  documents independent graphics/compute/copy queues, queue `Signal` / `Wait`, and the recommendation to keep each
  fence on one progress timeline.

## Context

- RHI D3D12 Per-Queue Fence Synchronization v1 completed backend-private per-queue fence timelines and public
  `FenceValue` source queue identity, removing the old global numeric fence timeline assumption.
- Runtime RHI Compute Morph Pipelined Scheduling D3D12 v1 can classify a previous-slot graphics consumption schedule
  as ready for backend-private timing, but there is not yet a D3D12 timestamp query path that records queue-local GPU
  intervals.
- Direct and compute queues are the first target because Microsoft documents timestamp support for
  `D3D12_COMMAND_LIST_TYPE_DIRECT` and `D3D12_COMMAND_LIST_TYPE_COMPUTE`; copy queue timestamp support is optional and
  remains out of scope unless a later plan validates `CopyQueueTimestampQueriesSupported`.

## Constraints

- Do not expose `ID3D12QueryHeap`, query readback buffers, native queues, native fences, command lists, events, or
  timestamp query values through gameplay, runtime-host, generated package, or backend-neutral RHI APIs.
- Do not claim async overlap, GPU speedup, package-visible performance readiness, Vulkan/Metal parity, frame graph
  scheduling, or generated package readiness in this slice.
- Do not add deterministic tests that require two hardware queues to overlap; hardware and driver scheduling may
  serialize even valid async submissions.
- Keep the implementation D3D12-primary and backend-private. NullRHI, Vulkan, and Metal only need to keep compiling
  unless this slice changes shared first-party contracts.

## File Map

- `engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp`: Add backend-specific queue timestamp diagnostic structs
  and `DeviceContext` inspection methods.
- `engine/rhi/d3d12/src/d3d12_backend.cpp`: Implement D3D12 query heap/readback recording for graphics and compute
  queues, using queue-local frequency and explicit fence waits.
- `tests/unit/d3d12_rhi_tests.cpp`: Add RED/GREEN focused tests for graphics and compute timestamp measurement
  support, failure diagnostics, and non-negative measured intervals.
- `docs/current-capabilities.md`, `docs/roadmap.md`, this plan registry, `engine/agent/manifest.json`,
  `.agents/skills/*`, `.claude/skills/*`, `.codex/agents/*`, `.claude/agents/*`, and `tools/check-ai-integration.ps1`:
  Keep the capability claim narrow and machine-readable.

## Done When

- A RED D3D12 test fails first because there is no backend-private queue timestamp measurement API for graphics and
  compute queues.
- D3D12 backend tests can query graphics and compute queue timestamp support independently and record a two-point GPU
  interval for each supported queue.
- Measurement diagnostics include queue kind, support status, frequency, begin/end ordering, elapsed seconds, and a
  clear diagnostic string without exposing native handles or raw package-visible timestamp counters.
- The implementation uses D3D12 timestamp query heaps and resolves query data to a readback buffer on the same queue
  that recorded the timestamps.
- The tests assert only support/ordering/finite conversion, not actual async overlap or performance improvement.
- Docs, manifest, plan registry, static checks, skills/subagents, and validation evidence describe this as a D3D12
  timestamp measurement foundation only.
- Focused D3D12 tests plus `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`,
  `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -MaxFiles 1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass, or
  concrete host/tool blockers are recorded.

## Tasks

- [x] Inspect existing `DeviceContext` queue creation, command list execution, readback mapping, and timestamp frequency
  code.
- [x] Add RED D3D12 backend tests for graphics/compute timestamp support and interval measurement.
- [x] Add backend-specific first-party timestamp diagnostic types to `d3d12_backend.hpp`.
- [x] Implement graphics/compute timestamp query heap recording, resolve, readback, and conversion in
  `d3d12_backend.cpp`.
- [x] Keep overlap classification conservative: no deterministic overlap assertion and no generated package or
  runtime-host performance counters.
- [x] Update docs, manifest, plan registry, static checks, skills/subagents, and validation evidence.

## Validation Evidence

- RED: Direct `cmake --build --preset dev --target mirakana_d3d12_rhi_tests` did not reach the intended compiler failure on
  this Windows host because MSBuild rejected duplicate `Path` / `PATH` environment entries; the repository wrapper was
  used for the actual RED check.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` failed in `tests/unit/d3d12_rhi_tests.cpp` because
  `DeviceContext::queue_timestamp_measurement_support`, `DeviceContext::measure_queue_timestamp_interval`, and
  `mirakana::rhi::d3d12::QueueTimestampMeasurementStatus` did not exist.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` passed after adding backend-specific first-party timestamp diagnostics plus D3D12 timestamp
  query heap recording, `ResolveQueryData`, queue-local fence wait, readback mapping, and floating-point seconds
  conversion.
- GREEN: `ctest --preset dev -R mirakana_d3d12_rhi_tests --output-on-failure` passed, including graphics and compute queue
  timestamp support and interval readback tests.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after adding static assertions for the timestamp plan, D3D12 backend diagnostic
  types, query heap/resolve implementation, and focused tests.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed after formatting the `ResolveQueryData` call site.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed; the new measurement surface is D3D12-backend-specific and does not add
  native D3D12/Win32 symbols to backend-neutral public headers.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -MaxFiles 1` passed.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed. Full CTest completed 29/29, including `mirakana_d3d12_rhi_tests`; Metal shader tools
  and Apple packaging remain diagnostic-only host blockers on this Windows machine.
