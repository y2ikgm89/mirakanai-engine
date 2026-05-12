# Runtime RHI Compute Morph Calibrated Overlap Diagnostics D3D12 v1 (2026-05-06)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `runtime-rhi-compute-morph-calibrated-overlap-diagnostics-d3d12-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Add a D3D12 backend-private calibrated overlap diagnostic that consumes the completed pipelined compute-morph schedule
evidence plus graphics/compute calibrated queue timing rows, so tests can classify measured CPU-QPC timeline overlap
without exposing native handles, raw query resources, generated package counters, or speedup claims.

## Architecture

Keep calibrated GPU/CPU timing rows in `mirakana_rhi_d3d12` and keep scheduling readiness in the existing first-party
`mirakana_rhi` diagnostics. Add a D3D12-specific diagnostic type/helper that accepts only first-party schedule diagnostics
and D3D12 calibrated rows, then reports a conservative classification for tests. Do not promote this to backend-neutral
RHI, `runtime_host`, generated package, editor, gameplay, or manifest validation counters in this slice.

## Tech Stack

C++23, `mirakana_rhi`, `mirakana_rhi_d3d12`, existing compute-morph pipelined scheduling tests, D3D12 timestamp queries,
`ID3D12CommandQueue::GetClockCalibration`, `QueryPerformanceCounter` correlation, and existing repository validation.

---

## Official References

- [Microsoft Learn: Timing (Direct3D 12 Graphics)](https://learn.microsoft.com/en-us/windows/win32/direct3d12/timing)
  documents per-queue timestamp frequencies, `GetClockCalibration` correlation with `QueryPerformanceCounter`, and
  floating-point seconds conversion for resolved timestamp ticks.
- [Microsoft Learn: Multi-engine synchronization](https://learn.microsoft.com/en-us/windows/win32/direct3d12/user-mode-heap-synchronization)
  documents queue `Signal`/`Wait`, one-timeline-per-fence guidance, multi-engine execution, and pipelined
  compute/graphics examples that require multiple buffered versions of compute-produced data.

## Context

- Runtime RHI Compute Morph Pipelined Scheduling D3D12 v1 already proves graphics can consume a previous output slot
  while compute writes a different current output slot, and reports
  `RhiAsyncOverlapReadinessStatus::ready_for_backend_private_timing`.
- RHI D3D12 Calibrated Queue Timing Diagnostics v1 already provides backend-private graphics and compute queue timing
  rows on one CPU-QPC timeline.
- The next honest step is classification, not a performance claim: the diagnostic may report measured overlap, measured
  non-overlap, or insufficient/failed timing evidence on the selected host path.

## Constraints

- Do not expose native D3D12 queues, fences, command lists, descriptor handles, query heaps, readback resources,
  `ID3D12CommandQueue`, GPU timestamp resources, or backend timing rows through gameplay, generated package,
  runtime-host, editor, or backend-neutral RHI public APIs.
- Do not add generated `DesktopRuntime3DPackage` counters, validation requirements, or package-visible timing metrics.
- Do not claim speedup, performance improvement, guaranteed parallelism, Vulkan/Metal parity, frame graph scheduling,
  directional-shadow morph rendering, scene-schema compute-morph authoring, graphics morph+skin composition, or broad
  renderer quality.
- Treat host scheduling as variable: tests may assert a valid classification and sane interval ordering, but must not
  require measured overlap on every GPU/driver.

## File Map

- `engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp`: Add D3D12-specific calibrated overlap diagnostic status,
  result struct, and helper declaration.
- `engine/rhi/d3d12/src/d3d12_backend.cpp`: Implement conservative classification from first-party readiness
  diagnostics plus calibrated graphics/compute queue timing intervals.
- `tests/unit/d3d12_rhi_tests.cpp`: Extend the pipelined compute-morph schedule test with RED/GREEN calibrated
  graphics/compute timing classification assertions.
- Docs, manifest, plan registry, static checks, skills/subagents, and `tools/check-ai-integration.ps1`: Keep the active
  claim narrow and machine-readable.

## Done When

- A RED D3D12 test fails first because no D3D12 calibrated compute/graphics overlap diagnostic type/helper exists.
- Focused tests build the existing two-slot compute morph schedule, capture calibrated graphics and compute timing rows,
  and classify them through a D3D12-specific helper.
- The diagnostic reports schedule-ready, timing-ready, measured-overlap, measured-non-overlap, or insufficient evidence
  without requiring measured overlap as a deterministic host invariant.
- Same-frame serial-dependency diagnostics and the existing pipelined scheduling readiness test still pass.
- Docs, manifest, plan registry, static checks, skills/subagents, and validation evidence describe this as
  backend-private diagnostics only, not generated package readiness or speedup.
- Focused tests plus `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`,
  `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -MaxFiles 1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass, or
  concrete host/tool blockers are recorded.

## Tasks

- [x] Inspect the completed `RhiPipelinedComputeGraphicsScheduleEvidence` path and D3D12 calibrated timing rows for the
  smallest backend-private overlap classifier.
- [x] Add a RED D3D12 test that calls the missing calibrated overlap diagnostic after the existing pipelined schedule
  proof captures graphics and compute timing rows.
- [x] Add D3D12-specific calibrated overlap status/result contracts in `d3d12_backend.hpp`.
- [x] Implement conservative overlap classification in `d3d12_backend.cpp`.
- [x] Preserve existing same-frame and pipelined schedule readiness diagnostics without adding generated package
  counters or runtime-host APIs.
- [x] Update docs, manifest, plan registry, static checks, skills/subagents, and validation evidence.

## Validation Evidence

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` failed in `tests/unit/d3d12_rhi_tests.cpp` because
  `mirakana::rhi::d3d12::measure_rhi_device_calibrated_queue_timing`,
  `mirakana::rhi::d3d12::diagnose_calibrated_compute_graphics_overlap`, and
  `mirakana::rhi::d3d12::QueueCalibratedOverlapStatus` did not exist.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` passed after adding `QueueCalibratedOverlapStatus`,
  `QueueCalibratedOverlapDiagnostics`, D3D12 `IRhiDevice` timing access, and the conservative classifier.
- GREEN: `ctest --preset dev -R mirakana_d3d12_rhi_tests --output-on-failure` passed, including the pipelined
  compute-morph calibrated overlap classification path.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed after applying clang-format to the touched D3D12 header/source/test files.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed after the backend-specific header change.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed with the updated active-plan, docs, skill, subagent, and code-surface assertions.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -MaxFiles 1` passed
  (`30977 warnings generated.` from the existing compile database smoke, exit 0).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed: 29/29 CTest tests passed, `validate: ok`; Metal shader tools and Apple packaging
  remained diagnostic-only host blockers on Windows.
