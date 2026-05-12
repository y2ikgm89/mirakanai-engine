# Runtime RHI Compute Morph Async Overlap Evidence D3D12 v1 (2026-05-06)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `runtime-rhi-compute-morph-async-overlap-evidence-d3d12-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Add a runtime/RHI-only D3D12 evidence slice that can prove or explicitly diagnose whether the existing compute morph
path is ready for measurable compute/graphics overlap, using first-party ordering and timing evidence while keeping
native queues, fences, command lists, timestamp resources, and backend diagnostics private.

## Architecture

Keep the work inside `mirakana_rhi`, `mirakana_rhi_d3d12`, renderer/runtime RHI tests, and backend-private diagnostics. Generated
gameplay and `DesktopRuntime3DPackage` package smoke must not consume overlap metrics in this slice. Any public surface
must be backend-neutral and first-party; native D3D12 objects, GPU timestamp heaps, query handles, command queues,
fences, descriptor handles, and PIX/debug-layer details stay backend-private.

## Tech Stack

C++23, `mirakana_rhi`, `mirakana_rhi_d3d12`, `mirakana_runtime_rhi`, D3D12 focused tests, official Microsoft D3D12 multi-engine,
synchronization, and timestamp/query guidance reviewed before implementation.

---

## Context

- Runtime RHI Compute Morph Queue Synchronization D3D12 v1 added queue-to-queue ordering through
  `IRhiDevice::wait_for_queue`.
- Runtime RHI Compute Morph Async Telemetry D3D12 v1 added first-party queue-submit and queue-wait fence sequencing
  telemetry.
- Generated 3D Compute Morph Async Telemetry Package Smoke D3D12 v1 promoted those sequencing counters into generated
  package validation through `--require-compute-morph-async-telemetry`.
- None of those slices prove actual GPU overlap, speedup, timestamp timing, or frame-graph scheduling.
- Official Microsoft D3D12 guidance reviewed for this slice:
  - [Multi-engine synchronization](https://learn.microsoft.com/en-us/windows/win32/direct3d12/user-mode-heap-synchronization)
    documents separate 3D/compute/copy queues, fence `Signal`/`Wait`, queue-to-queue synchronization, and pipelined
    compute/graphics examples that require multiple versions of the data crossing from compute to graphics.
  - [Timing](https://learn.microsoft.com/en-us/windows/win32/direct3d12/timing) documents per-queue timestamp frequency,
    direct/compute timestamp support, timestamp queries, and `ResolveQueryData` conversion rules.
  - [ID3D12GraphicsCommandList::ResolveQueryData](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist-resolvequerydata)
    documents safe query resolution requirements; unresolved or incomplete queries must not be resolved.

## Outcome

This slice adds `mirakana::rhi::RhiAsyncOverlapReadinessDiagnostics`,
`mirakana::rhi::RhiAsyncOverlapReadinessStatus`, and
`mirakana::rhi::diagnose_compute_graphics_async_overlap_readiness`. The diagnostic consumes only first-party
`RhiStats` queue-submit/queue-wait fence evidence plus backend-reported timestamp-frequency availability; it does not
expose native D3D12 queues, fences, command lists, query heaps, timestamp resources, or timestamp values.

The D3D12 compute morph async path now deterministically reports
`RhiAsyncOverlapReadinessStatus::not_proven_serial_dependency` for the current same-frame pattern where graphics waits
on the last compute fence before submitting dependent graphics work. That is an honest unsupported diagnostic, not a
performance or overlap claim. Follow-up work should add a pipelined/multi-output-slot compute morph path before any
timed overlap proof is attempted.

## Constraints

- Do not expose native D3D12 queue/fence handles, command lists, timestamp/query heap handles, descriptor handles,
  swapchain frames, backend stats, PIX/debug-layer data, or GPU timestamp data through game/runtime-host public APIs.
- Do not update generated package validation to require overlap or timing in this slice.
- Do not claim async speedup, performance improvement, Vulkan/Metal parity, frame graph scheduling, directional-shadow
  morph rendering, scene-schema compute-morph authoring, graphics morph+skin composition, or broad renderer quality.
- Prefer an explicit "not proven on this host/path" diagnostic over inferred overlap.
- Review official Microsoft D3D12 documentation before implementing timestamp or multi-engine evidence.

## Done When

- A RED D3D12/runtime-RHI test or static check fails first because no overlap-readiness evidence/diagnostic contract
  exists for the compute morph async path.
- The D3D12 compute morph async path can produce first-party, backend-private evidence that either proves ordered
  compute/graphics overlap readiness on a host/toolchain that supports it or reports a deterministic unsupported
  diagnostic.
- Public/runtime-host surfaces still expose only first-party backend-neutral counters/diagnostics and no native handles
  or GPU timestamp resources.
- Docs, manifest, plan registry, static checks, and skills describe this as runtime/RHI D3D12 overlap evidence only, not
  package-visible performance.
- Focused tests plus `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, validate-scoped tidy, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`
  pass, or concrete host/tool blockers are recorded.

## Tasks

- [x] Review official D3D12 multi-engine synchronization and timestamp/query guidance relevant to compute/graphics
      overlap evidence.
- [x] Inspect existing D3D12 compute morph queue-submit, queue-wait, and telemetry implementation boundaries.
- [x] Add a RED D3D12/runtime-RHI test or static check for missing overlap-readiness evidence/diagnostics.
- [x] Implement minimal first-party evidence or deterministic unsupported diagnostics without exposing native handles.
- [x] Update docs, manifest, plan registry, static checks, skills/subagents, and validation evidence.

## Validation Evidence

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` failed while compiling `mirakana_d3d12_rhi_tests` because
  `mirakana::rhi::diagnose_compute_graphics_async_overlap_readiness` and
  `mirakana::rhi::RhiAsyncOverlapReadinessStatus` did not exist yet.
- GREEN focused build: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` passed after adding `engine/rhi/src/async_overlap.cpp`, the backend-neutral
  diagnostic contract, and D3D12/RHI tests.
- GREEN focused tests: `ctest --preset dev -R "mirakana_(rhi|d3d12_rhi)_tests" --output-on-failure` passed.
- Static integration: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed.
- Targeted tidy: direct clang-tidy on `engine/rhi/src/async_overlap.cpp` passed after adding the required direct
  `<cstdint>` include. A full `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1` reached existing broad repo warnings and timed out after 300s, so
  full-repo tidy is not claimed for this slice.
- Full validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed; validation reported diagnostic-only Metal/Apple host blockers, built the
  `dev` preset, and CTest passed 29/29 tests including `mirakana_rhi_tests` and `mirakana_d3d12_rhi_tests`.
