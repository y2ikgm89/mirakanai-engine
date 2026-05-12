# Runtime RHI Compute Morph Pipelined Scheduling D3D12 v1 (2026-05-06)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `runtime-rhi-compute-morph-pipelined-scheduling-d3d12-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Use the completed compute morph output ring to prove a D3D12 runtime/RHI scheduling pattern where graphics consumes a
previously completed output slot while compute writes a different current slot, avoiding the current same-frame
graphics wait on the newest compute fence without claiming measured overlap, speedup, or generated package readiness.

## Architecture

Keep the proof inside `mirakana_rhi`, `mirakana_runtime_rhi`, D3D12 focused tests, and first-party diagnostics. Use public RHI queue
submission and `IRhiDevice::wait_for_queue` only for explicit queue ordering on already selected fences. Do not expose
native queues, fences, command lists, descriptor handles, query heaps, timestamp resources, backend stats, or D3D12
objects to gameplay or runtime-host public APIs.

## Tech Stack

C++23, `mirakana_rhi`, `mirakana_runtime_rhi`, `mirakana_rhi_d3d12`, D3D12 focused tests, and official Microsoft D3D12 multi-engine
synchronization guidance for multiple versions of compute-produced graphics data.

---

## Context

- Runtime RHI Compute Morph Async Overlap Evidence D3D12 v1 added
  `RhiAsyncOverlapReadinessDiagnostics`, which reports `not_proven_serial_dependency` when graphics waits on the same
  compute fence submitted for the current frame.
- Runtime RHI Compute Morph Pipelined Output Ring D3D12 v1 added `output_slot_count`,
  `RuntimeMorphMeshComputeOutputSlot`, and distinct POSITION output slots/descriptors so compute and graphics can select
  different buffers.
- The remaining scheduling gap is to prove a first-party pattern that waits graphics on a previously completed slot
  fence while current compute writes another slot.

## Constraints

- Do not claim async speedup, measured overlap, GPU performance improvement, Vulkan/Metal parity, frame graph
  scheduling, directional-shadow morph rendering, scene-schema compute-morph authoring, graphics morph+skin composition,
  generated package readiness, or broad renderer quality.
- Do not update generated `DesktopRuntime3DPackage` validation or package-visible performance counters in this slice.
- Keep timestamp/query resources backend-private; this slice can make readiness diagnostics more precise, but must not
  expose GPU timestamp values through gameplay/runtime-host APIs.
- Keep the output-ring contract first-party and runtime/RHI-owned.

## Done When

- A RED D3D12/runtime-RHI test fails first because there is no first-party pipelined compute/graphics scheduling proof
  using distinct output slots and previous-slot fences.
- Focused tests prove graphics can consume slot N-1 while compute writes slot N, and the readiness diagnostic no longer
  classifies that pattern as `not_proven_serial_dependency` when backend-private timestamp frequency evidence is
  available.
- Existing same-frame serial-dependency diagnostics and single-slot compute morph tests still pass.
- Docs, manifest, plan registry, static checks, skills/subagents, and validation evidence describe this as scheduling
  readiness only, not measured overlap or package-visible performance.
- Focused tests plus `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, validate-scoped tidy, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`
  pass, or concrete host/tool blockers are recorded.

## Tasks

- [x] Inspect current compute morph queue wait and output-ring tests for a minimal two-slot scheduling proof.
- [x] Add a RED D3D12/runtime-RHI test for graphics consuming a previous output slot while current compute writes another.
- [x] Implement first-party scheduling diagnostics/helpers needed for the pipelined pattern without native handles.
- [x] Preserve the existing same-frame `not_proven_serial_dependency` diagnostic path.
- [x] Update docs, manifest, plan registry, static checks, skills/subagents, and validation evidence.

## Validation Evidence

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` failed after adding RHI and D3D12 tests because
  `mirakana::rhi::RhiPipelinedComputeGraphicsScheduleEvidence` and
  `mirakana::rhi::diagnose_pipelined_compute_graphics_async_overlap_readiness` did not exist.
- GREEN focused build: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` passed after adding the first-party output-ring schedule evidence struct,
  pipelined diagnostic fields, and the backend-neutral schedule readiness helper.
- GREEN focused tests: `ctest --preset dev -R "mirakana_(rhi|d3d12_rhi)_tests" --output-on-failure` passed. The D3D12
  test dispatches compute into slot 0, dispatches the next compute into slot 1, waits the graphics queue on the slot 0
  fence, copies slot 0 on the graphics queue, and verifies the diagnostic reports
  `ready_for_backend_private_timing` rather than `not_proven_serial_dependency`.
- Static checks: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed after the
  docs/manifest/skill/subagent updates.
- Full validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed. It reported `tidy-check: ok (1 files)`, rebuilt the `dev` preset, and
  passed CTest `29/29`. Metal shader packaging and Apple packaging remained Windows-host diagnostic-only blockers.
