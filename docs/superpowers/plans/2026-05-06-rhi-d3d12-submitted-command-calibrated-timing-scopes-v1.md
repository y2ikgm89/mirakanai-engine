# RHI D3D12 Submitted Command Calibrated Timing Scopes v1 (2026-05-06)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `rhi-d3d12-submitted-command-calibrated-timing-scopes-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Record backend-private calibrated timing rows for actual submitted D3D12 graphics/compute command lists so the
pipelined compute-morph overlap diagnostic can classify real submitted workload intervals instead of separate empty
queue timing probes.

## Architecture

Keep all query heaps, readback resources, raw timestamp ticks, clock calibration samples, native queues, and command-list
handles inside `mirakana_rhi_d3d12`. Instrument D3D12 graphics/compute command lists with begin/end timestamp queries around
their recorded commands, resolve those timestamps into backend-private readback rows, then expose only D3D12-specific
first-party timing structs through the existing backend header and tests. Reuse the existing calibrated overlap
classifier boundary without adding backend-neutral RHI, runtime-host, generated package, gameplay, editor, or performance
claim surfaces.

## Tech Stack

C++23, `mirakana_rhi_d3d12`, `IRhiDevice::submit`, D3D12 timestamp query heaps, `ID3D12GraphicsCommandList::EndQuery`,
`ResolveQueryData`, `ID3D12CommandQueue::GetClockCalibration`, `QueryPerformanceCounter` frequency conversion, and
existing D3D12 compute-morph pipelined scheduling tests.

---

## Official References

- [Microsoft Learn: Timing (Direct3D 12 Graphics)](https://learn.microsoft.com/windows/win32/direct3d12/timing)
  documents per-command-queue timestamp frequency, timestamp queries recorded in command lists, bottom-of-pipe timestamp
  behavior, `GetClockCalibration` correlation with `QueryPerformanceCounter`, and floating-point interval conversion.
- [Microsoft Learn: D3D12_QUERY_HEAP_TYPE](https://learn.microsoft.com/windows/win32/api/d3d12/ne-d3d12-d3d12_query_heap_type)
  documents regular timestamp query heaps for direct/compute command lists and separate copy-queue timestamp heap
  requirements.

## Context

- Runtime RHI Compute Morph Calibrated Overlap Diagnostics D3D12 v1 is complete, but its timing rows are separate
  backend-private empty queue probes rather than the submitted compute and graphics command lists from the pipelined
  schedule proof.
- D3D12 graphics and compute command lists already flow through `D3d12RhiDevice::submit`, which validates ownership,
  closes lists before submit, records queue/fence stats, and waits through first-party `FenceValue`.
- The honest next step is backend-private measurement of submitted command-list intervals. The diagnostic may classify
  measured overlap or measured non-overlap, and tests must not require overlap on every host.

## Constraints

- Do not expose native D3D12 queues, command lists, query heaps, readback resources, descriptor handles, fences, raw GPU
  timestamps, raw CPU timing samples, calibration math, or backend timing rows through backend-neutral RHI, gameplay,
  generated packages, runtime-host, editor, or public package validation counters.
- Do not claim speedup, guaranteed parallelism, package-visible measured async compute overlap, Vulkan/Metal parity,
  frame graph scheduling, directional-shadow morph rendering, scene-schema compute-morph authoring, graphics morph+skin
  composition, or broad renderer quality.
- Keep copy queue submitted-command timing unsupported in this slice unless the feature is separately probed and
  validated through the documented copy queue timestamp heap path.
- Preserve existing D3D12 command-list ownership, close, submit, reset, swapchain-frame release, and queue/fence ordering
  behavior.

## File Map

- `engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp`: Add D3D12-specific submitted-command calibrated timing
  status/result contracts and helper declarations.
- `engine/rhi/d3d12/src/d3d12_backend.cpp`: Allocate backend-private timestamp query/readback resources for D3D12
  graphics/compute command lists, record begin/end timestamps around submitted commands, resolve/map completed rows, and
  expose D3D12-only helper functions through the existing dynamic-cast bridge.
- `tests/unit/d3d12_rhi_tests.cpp`: Extend the pipelined compute-morph scheduling test with RED/GREEN submitted compute
  and graphics command timing assertions and classify those actual intervals.
- Docs, manifest, plan registry, static checks, skills/subagents, and `tools/check-ai-integration.ps1`: Move the
  calibrated-overlap slice to completed and keep this new active slice narrow and machine-readable.

## Done When

- A RED D3D12 test fails first because submitted-command calibrated timing contracts/helpers do not exist.
- D3D12 graphics/compute command lists record timestamp begin/end rows around their actual commands, resolve those rows,
  and can be read after the returned first-party `FenceValue` completes.
- The pipelined compute-morph schedule test classifies the actual current-compute and graphics-copy submitted command
  intervals through backend-private D3D12 timing rows without requiring measured overlap.
- Existing queue timestamp, clock calibration, calibrated queue timing, calibrated overlap, per-queue fence, and
  compute-morph scheduling tests still pass.
- Docs, manifest, plan registry, static checks, skills/subagents, and validation evidence describe this as backend-private
  submitted-command timing only.
- Focused tests plus `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`, `ctest --preset dev -R mirakana_d3d12_rhi_tests --output-on-failure`,
  `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`,
  `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -MaxFiles 1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass, or
  concrete host/tool blockers are recorded.

## Tasks

- [x] Inspect D3D12 `CommandListRecord`, `DeviceContext::close_command_list`, `DeviceContext::execute_command_list`, and
  `D3d12RhiDevice::submit` for the smallest backend-private submitted-command timing hook.
- [x] Add a RED D3D12 test that reads calibrated timing rows for the actual current compute command and graphics command
  returned by the pipelined compute-morph schedule proof.
- [x] Add D3D12-specific `SubmittedCommandCalibratedTimingStatus` /
  `SubmittedCommandCalibratedTiming` contracts and helper declarations.
- [x] Instrument graphics/compute command lists with backend-private timestamp query resources, begin/end query recording,
  resolve, fence-completion checks, readback mapping, and calibration conversion.
- [x] Add an overload or bridge that lets the existing calibrated overlap classifier consume submitted-command timing rows.
- [x] Preserve existing command-list submit/reset behavior and keep copy-queue timing unsupported for this slice.
- [x] Update docs, manifest, plan registry, static checks, skills/subagents, and validation evidence.

## Validation Evidence

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` failed in `tests/unit/d3d12_rhi_tests.cpp` because
  `mirakana::rhi::d3d12::read_rhi_device_submitted_command_calibrated_timing` and
  `mirakana::rhi::d3d12::SubmittedCommandCalibratedTimingStatus` did not exist.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` passed after adding `SubmittedCommandCalibratedTimingStatus`,
  `SubmittedCommandCalibratedTiming`, D3D12 command-list timestamp instrumentation, readback conversion, and the
  overlap classifier bridge.
- GREEN: `ctest --preset dev -R mirakana_d3d12_rhi_tests --output-on-failure` passed, including the actual submitted
  current-compute and graphics-copy timing classification path.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed after applying clang-format to the touched D3D12 header/source/test files.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed after the backend-specific header change.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed with the updated active-plan, docs, skill, subagent, and code-surface assertions.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -MaxFiles 1` passed
  (`30977 warnings generated.` from the existing compile database smoke, exit 0).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed: 29/29 CTest tests passed, `validate: ok`; Metal shader tools and Apple packaging
  remained diagnostic-only host blockers on Windows.
