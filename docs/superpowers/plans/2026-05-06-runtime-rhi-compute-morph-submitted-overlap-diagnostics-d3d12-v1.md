# Runtime RHI Compute Morph Submitted Overlap Diagnostics D3D12 v1 (2026-05-06)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `runtime-rhi-compute-morph-submitted-overlap-diagnostics-d3d12-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Centralize the D3D12 compute/graphics submitted-command calibrated timing readback and overlap classification behind a
single D3D12-specific helper so runtime/RHI tests can ask for actual submitted compute/graphics fences overlap
diagnostics without manually exposing or stitching timing rows.

## Architecture

Keep timing rows, timestamp readbacks, raw GPU ticks, clock calibration samples, and fence-to-command-list lookup inside
`mirakana_rhi_d3d12`. Add one D3D12-specific helper that accepts an `IRhiDevice`, the existing
`RhiAsyncOverlapReadinessDiagnostics`, the submitted current-compute fence, and the submitted graphics fence; the helper
reads backend-private submitted-command timing rows and returns only the existing `QueueCalibratedOverlapDiagnostics`.
The helper does not add backend-neutral RHI, runtime-host, gameplay, package, editor, or performance-claim surfaces.

## Tech Stack

C++23, `mirakana_rhi_d3d12`, `IRhiDevice::submit`, first-party `FenceValue` queue identity,
`read_rhi_device_submitted_command_calibrated_timing`, existing calibrated overlap classifier, and the D3D12 pipelined
compute-morph scheduling test.

---

## Official References

- [Microsoft Learn: Timing (Direct3D 12 Graphics)](https://learn.microsoft.com/windows/win32/direct3d12/timing)
  documents timestamp query intervals, queue timestamp frequency, `GetClockCalibration`, and fence-gated readback.
- [Microsoft Learn: D3D12_QUERY_HEAP_TYPE](https://learn.microsoft.com/windows/win32/api/d3d12/ne-d3d12-d3d12_query_heap_type)
  documents regular timestamp query heaps for direct/compute command lists and the separate copy-queue timestamp heap
  path that remains unsupported in this slice.

## Context

- RHI D3D12 Submitted Command Calibrated Timing Scopes v1 is complete and can read calibrated timing rows for actual
  submitted graphics/compute command lists after their returned `FenceValue` completes.
- The current pipelined compute-morph scheduling test still manually calls
  `read_rhi_device_submitted_command_calibrated_timing` for compute and graphics fences, then manually calls
  `diagnose_calibrated_compute_graphics_overlap`.
- The next production-safe boundary is a D3D12-only helper that keeps those timing rows backend-private while exposing
  the already narrow overlap diagnostic value.

## Constraints

- Do not expose native D3D12 queues, command lists, fences, query heaps, readback resources, descriptor handles, raw GPU
  timestamps, raw CPU/QPC timing samples, calibration math, or submitted-command timing rows through backend-neutral RHI,
  gameplay, generated packages, runtime-host, editor, validation recipes, or public package counters.
- Do not claim speedup, guaranteed overlap, generated-package measured async compute overlap, Vulkan/Metal parity, frame
  graph scheduling, directional-shadow morph rendering, scene-schema compute-morph authoring, graphics morph+skin
  composition, or broad renderer quality.
- Do not add implicit CPU waits to the helper. Callers must still wait for returned fences when they need ready timing;
  incomplete fences must remain missing timing evidence.
- Keep copy-queue submitted timing unsupported in this slice.

## File Map

- `engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp`: Declare the D3D12-only submitted-command overlap helper.
- `engine/rhi/d3d12/src/d3d12_backend.cpp`: Implement the helper by reading the two submitted timing rows and forwarding
  them to the existing calibrated overlap classifier.
- `tests/unit/d3d12_rhi_tests.cpp`: Replace manual timing-row stitching in the pipelined compute-morph schedule test
  with the helper and keep readiness assertions on the returned diagnostic.
- Docs, manifest, plan registry, static checks, skills/subagents, and `tools/check-ai-integration.ps1`: Mark submitted
  command timing as complete and keep this active slice narrow and machine-readable.

## Done When

- A RED D3D12 test fails first because
  `diagnose_rhi_device_submitted_command_compute_graphics_overlap` does not exist.
- The helper accepts `IRhiDevice`, schedule diagnostics, current compute fence, and graphics fence, then returns
  `QueueCalibratedOverlapDiagnostics` from backend-private submitted-command timing rows.
- The pipelined compute-morph schedule test no longer manually reads or stitches submitted-command timing rows.
- Existing submitted-command timing, calibrated overlap, calibrated queue timing, queue clock, queue timestamp, per-queue
  fence, and compute-morph scheduling tests still pass.
- Docs, manifest, plan registry, static checks, skills/subagents, and validation evidence describe this as D3D12-only
  backend-private submitted-command overlap diagnostics.
- Focused tests plus `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`, `ctest --preset dev -R mirakana_d3d12_rhi_tests --output-on-failure`,
  `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`,
  `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -MaxFiles 1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass, or
  concrete host/tool blockers are recorded.

## Tasks

- [x] Add a RED D3D12 test that calls
  `mirakana::rhi::d3d12::diagnose_rhi_device_submitted_command_compute_graphics_overlap`.
- [x] Declare the D3D12-only helper in the existing backend header.
- [x] Implement the helper by reading compute/graphics submitted-command calibrated timing rows and reusing the existing
  classifier.
- [x] Remove manual submitted-timing row stitching from the pipelined compute-morph scheduling test.
- [x] Update docs, manifest, plan registry, static checks, skills/subagents, and validation evidence.

## Validation Evidence

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` failed in `tests/unit/d3d12_rhi_tests.cpp` because
  `mirakana::rhi::d3d12::diagnose_rhi_device_submitted_command_compute_graphics_overlap` did not exist.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` passed after adding the D3D12-only helper declaration/implementation and replacing manual
  test-side timing-row stitching with the helper call.
- GREEN: `ctest --preset dev -R mirakana_d3d12_rhi_tests --output-on-failure` passed.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after updating the active-plan pointer, docs, skills/subagents, and static
  assertions.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed after applying clang-format to the touched D3D12 header/source/test files.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed after the backend-specific header helper declaration.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -MaxFiles 1` passed
  (`30977 warnings generated.` from the existing compile database smoke, exit 0).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed: 29/29 CTest tests passed, `validate: ok`; Metal shader tools and Apple packaging
  remained diagnostic-only host blockers on Windows.
