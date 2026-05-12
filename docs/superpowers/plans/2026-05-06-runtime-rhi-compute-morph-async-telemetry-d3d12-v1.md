# Runtime RHI Compute Morph Async Telemetry D3D12 v1 (2026-05-06)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `runtime-rhi-compute-morph-async-telemetry-d3d12-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Add narrow D3D12 compute-morph async telemetry that proves host-owned compute submissions, graphics queue waits, and
fence sequencing are observable through first-party counters/diagnostics without claiming performance overlap, async
speedup, Vulkan/Metal parity, public native handles, or generated gameplay access to queues/fences.

## Architecture

Keep telemetry in RHI/renderer/runtime-host adapters. Public surfaces may expose backend-neutral counters or diagnostic
names only; native D3D12 queues, fences, command lists, descriptor handles, swapchain frames, and timestamp objects remain
private backend/host details.

## Tech Stack

C++23, `mirakana_rhi`, `mirakana_rhi_d3d12`, `mirakana_runtime_rhi`, `mirakana_runtime_host_sdl3_presentation`, NullRHI validation, D3D12
focused tests, static docs checks.

---

## Context

- Runtime RHI Compute Morph Queue Synchronization D3D12 v1 added `IRhiDevice::wait_for_queue` and queue-wait stats.
- Generated 3D Compute Morph Queue Sync Package Smoke D3D12 v1 made `scene_gpu_compute_morph_queue_waits`
  package-visible.
- Runtime/renderer and runtime-scene slices now prove skin+compute composition and generated package counters, but
  async overlap/performance claims remain unsupported.

## Constraints

- Do not expose native D3D12 queue/fence handles or command lists through public engine/game APIs.
- Do not claim async overlap, GPU performance improvement, Vulkan/Metal parity, frame graph scheduling, or broad renderer
  quality.
- Keep generated gameplay limited to first-party counters/diagnostics if this slice reaches the package lane later.
- Preserve existing compute morph POSITION/NORMAL/TANGENT and skin+compute package smoke behavior.

## Done When

- A RED focused test fails first because compute-morph queue/fence sequencing telemetry is not observable enough.
- NullRHI and D3D12 focused tests prove backend-neutral telemetry rows/counters without native handle exposure.
- Docs, manifest, plan registry, static checks, and skills describe this as D3D12 async telemetry only.
- Focused tests plus `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, validate-scoped tidy, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`
  pass, or concrete host/tool blockers are recorded.

## Tasks

- [x] Inspect existing compute morph dispatch, queue wait, fence, and stats flow.
- [x] Add a RED focused test for missing compute-morph async telemetry.
- [x] Implement minimal backend-neutral telemetry and D3D12 private sequencing evidence.
- [x] Keep generated/package surfaces first-party and non-native if any package counter is added.
- [x] Update docs, manifest, plan registry, static checks, skills/subagents, and validation evidence.

## Validation Evidence

- RED: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_rhi_tests"` failed
  because `mirakana::rhi::RhiStats` did not expose `compute_queue_submits`, `graphics_queue_submits`,
  `last_compute_submitted_fence_value`, `last_graphics_queue_wait_fence_value`,
  `last_queue_wait_fence_value`, or `last_graphics_submitted_fence_value`.
- GREEN focused checks passed:
  - `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_rhi_tests"`
  - `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_d3d12_rhi_tests"`
  - `cmd.exe /d /c "set Path=%Path%&& set PATH=&& ctest --preset dev -R mirakana_rhi_tests --output-on-failure"`
  - `cmd.exe /d /c "set Path=%Path%&& set PATH=&& ctest --preset dev -R mirakana_d3d12_rhi_tests --output-on-failure"`
- Static/quality checks passed:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -MaxFiles 1`
- Final validation passed:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`
