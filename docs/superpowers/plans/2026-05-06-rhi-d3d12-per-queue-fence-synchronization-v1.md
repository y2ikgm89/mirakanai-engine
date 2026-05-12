# RHI D3D12 Per-Queue Fence Synchronization v1 (2026-05-06)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `rhi-d3d12-per-queue-fence-synchronization-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Bring the D3D12 RHI queue synchronization model in line with official multi-engine synchronization practice by replacing
the current single shared submission fence with backend-private per-queue fences while keeping public synchronization
backend-neutral and first-party.

## Architecture

Keep native `ID3D12Fence`, queue handles, events, and fence arrays private to `mirakana_rhi_d3d12`. Public RHI may carry
first-party queue identity on `FenceValue` so `IRhiDevice::wait`, `IRhiDevice::wait_for_queue`, and diagnostics can
target the correct source queue without exposing native handles. Do not change gameplay/runtime-host APIs to accept
D3D12 objects.

## Tech Stack

C++23, `mirakana_rhi`, `mirakana_rhi_d3d12`, `mirakana_rhi_vulkan`/`mirakana_rhi_metal` compile preservation, NullRHI tests, D3D12 focused
tests, and Microsoft D3D12 multi-engine fence synchronization guidance.

---

## Context

- Runtime RHI Compute Morph Pipelined Scheduling D3D12 v1 proves graphics can consume a previous output slot while
  compute writes a different slot, but D3D12 still uses one backend fence object and one global numeric sequence across
  graphics, compute, and copy queues.
- Official D3D12 multi-engine synchronization is clearer when each engine has its own fence timeline and cross-queue
  waits identify the source fence timeline explicitly.
- Future measured overlap or stronger async claims should not be built on ambiguous cross-queue fence values.

## Constraints

- Do not expose native fences, native queues, command lists, events, descriptor handles, timestamp queries, or D3D12
  objects through gameplay, runtime-host, or generated package APIs.
- Do not claim measured async overlap, GPU speedup, package-visible performance readiness, Vulkan/Metal parity, frame
  graph scheduling, or generated package readiness in this slice.
- Preserve existing public call shapes where a clean first-party `FenceValue` extension is sufficient; breaking internal
  assumptions is allowed when they block correct per-queue synchronization.
- Keep Vulkan/Metal and NullRHI compiling with equivalent first-party fence identity semantics even if they remain
  single-lane internally.

## Done When

- A RED RHI/D3D12 test fails first because submitted fences do not carry queue identity and D3D12 queue waits cannot
  prove they wait the source queue's fence timeline.
- `FenceValue` carries first-party queue identity, `submit` returns the submitting queue, and `wait_for_queue` waits the
  source fence timeline on the destination queue without exposing native handles.
- D3D12 focused tests prove compute, graphics, and copy queues can each report fence value `1` on their own timelines,
  and that graphics can wait on a compute fence whose numeric value collides with a graphics fence.
- Existing queue stats, invalid-wait diagnostics, compute morph queue sync, pipelined scheduling diagnostics, NullRHI,
  Vulkan, and Metal compile paths remain coherent.
- Docs, manifest, plan registry, static checks, skills/subagents, and validation evidence describe this as RHI
  synchronization hardening only, not measured overlap or package performance.
- Focused tests plus `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, validate-scoped tidy, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`
  pass, or concrete host/tool blockers are recorded.

## Tasks

- [x] Inspect `FenceValue`, NullRHI, D3D12 queue/fence implementation, and existing queue wait tests.
- [x] Add RED NullRHI/D3D12 tests for queue identity and colliding per-queue fence values.
- [x] Extend `FenceValue` and backend-neutral wait validation with source queue identity.
- [x] Replace the D3D12 single shared submission fence with backend-private per-queue fence timelines.
- [x] Preserve Vulkan/Metal compile behavior and update queue stats/diagnostics as needed.
- [x] Update docs, manifest, plan registry, static checks, skills/subagents, and validation evidence.

## Validation Evidence

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` failed after adding NullRHI/D3D12 queue-identity tests because `mirakana::rhi::FenceValue` had no
  public `queue` member.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` passed after adding first-party `FenceValue::queue`, NullRHI per-queue timelines, D3D12
  backend-private per-queue fences, queue wait source identity, and queue event ordering telemetry.
- GREEN: `ctest --preset dev -R "mirakana_(rhi|d3d12_rhi)_tests" --output-on-failure` passed; the focused tests prove
  graphics/compute/copy fence value collisions at `1`, graphics waiting on a compute fence with the same numeric value
  as a graphics fence, and async-overlap diagnostics using queue identity plus event order instead of a single numeric
  timeline.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, and
  `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -MaxFiles 1` passed.
- BLOCKED: full `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1` exceeded the local 600s timeout while analyzing the broader repository and
  emitting existing unrelated warnings; the validate-scoped `-MaxFiles 1` tidy lane passed.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed. It reported `tidy-check: ok (1 files)`, rebuilt the `dev` preset, and ran CTest
  `29/29` passed. Metal shader tools and Apple packaging remain diagnostic-only blockers on this Windows host.
