# Runtime RHI Compute Morph Queue Synchronization D3D12 v1 (2026-05-06)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `runtime-rhi-compute-morph-queue-synchronization-d3d12-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Add the first backend-neutral RHI queue-to-queue synchronization contract needed for D3D12 compute morph scheduling: compute queue dispatch produces morph output, graphics queue waits on that fence, and the renderer consumes the output without host-side `IRhiDevice::wait` standing in for GPU queue ordering.

## Architecture

Expose an explicit first-party queue wait API on `IRhiDevice` that maps to existing D3D12 `ID3D12CommandQueue::Wait` behavior and deterministic NullRHI validation. Keep queue/fence details backend-neutral; do not expose native D3D12 queue, fence, command-list, descriptor, or resource handles.

## Tech Stack

C++23, `mirakana_rhi`, `mirakana_rhi_d3d12`, `mirakana_runtime_rhi`, D3D12 native tests, NullRHI contract tests, generated documentation/static checks.

---

## Context

- `rhi-compute-dispatch-foundation-v1` added public compute command lists, compute pipelines, descriptor binding, and dispatch.
- `runtime-rhi-compute-morph-d3d12-proof-v1` proved direct D3D12 compute morph POSITION output readback.
- `runtime-rhi-compute-morph-renderer-consumption-d3d12-v1` currently dispatches compute, waits for completion on the host, then draws the compute-written POSITION output.
- `runtime-rhi-compute-morph-normal-tangent-output-d3d12-v1` and `generated-3d-compute-morph-normal-tangent-package-smoke-d3d12-v1` extend the output streams to POSITION/NORMAL/TANGENT.
- `mirakana::rhi::d3d12::DeviceContext::queue_wait_for_fence` already proves the native D3D12 queue wait primitive privately; the public `IRhiDevice` contract still lacks a backend-neutral queue wait.

## Constraints

- Do not claim async compute overlap, GPU performance improvement, Vulkan/Metal parity, frame-graph scheduling, skin+morph composition, directional-shadow morph rendering, scene-schema compute morph authoring, or broad renderer quality.
- Keep native `ID3D12CommandQueue`, `ID3D12Fence`, command allocator/list, descriptor heap, and resource handles private to `engine/rhi/d3d12`.
- Preserve the existing host-side `IRhiDevice::wait(FenceValue)` API for CPU synchronization; add the new queue wait as an explicit GPU queue dependency.
- NullRHI must validate invalid unsignaled fences and track stats deterministically.
- D3D12 `IRhiDevice` must route the queue wait through the existing private `DeviceContext::queue_wait_for_fence` primitive.

## Done When

- A NullRHI-focused RED test fails first because `IRhiDevice` has no queue wait contract or queue-wait stats.
- A D3D12 `IRhiDevice` RED test fails first because graphics queue cannot wait on a compute queue fence through public RHI APIs.
- `RhiStats` exposes queue wait counts/failures without native backend details.
- `NullRhiDevice` implements and validates `wait_for_queue(QueueKind queue, FenceValue fence)` deterministically.
- `D3d12RhiDevice` implements the same API by calling the backend-private queue wait primitive.
- A D3D12 compute morph renderer-consumption test submits compute queue work, calls the new graphics queue wait, submits graphics draw work, reads back the visible result, and does not call `IRhiDevice::wait` between compute submit and graphics submit except for final readback completion.
- Docs, manifest, plan registry, static checks, and rendering skills describe this as queue synchronization only, not async overlap.
- Focused tests plus `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, validate-scoped tidy, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass, or concrete host/tool blockers are recorded.

## Tasks

- [x] Inspect current `IRhiDevice::submit`/`wait`, `RhiStats`, NullRHI fence handling, D3D12 public device wrapper, and D3D12 private `DeviceContext::queue_wait_for_fence`.
- [x] Add a NullRHI RED test for `wait_for_queue(QueueKind::graphics, compute_fence)` stats and invalid unsignaled-fence failure.
- [x] Add a D3D12 public RHI RED test that submits compute queue morph output and requires graphics queue to wait on that fence before drawing.
- [x] Add `RhiStats::queue_waits` and `RhiStats::queue_wait_failures`; update NullRHI and D3D12 stats mappings.
- [x] Add `IRhiDevice::wait_for_queue(QueueKind queue, FenceValue fence)` with NullRHI deterministic validation and D3D12 private queue wait routing.
- [x] Update compute morph renderer-consumption tests to prove GPU queue ordering without a CPU wait between compute and graphics submissions.
- [x] Update docs, manifest, plan registry, static checks, skills/subagents, and validation evidence.

## Validation Evidence

- RED: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_rhi_tests"` failed because `mirakana::rhi::NullRhiDevice::wait_for_queue` and `RhiStats::queue_waits` / `queue_wait_failures` did not exist.
- RED: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& ctest --preset dev -R mirakana_d3d12_rhi_tests --output-on-failure"` failed in `d3d12 rhi frame renderer consumes compute morph output positions` because the test expected a graphics queue wait but the path still used `IRhiDevice::wait(compute_fence)`.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_rhi_tests"` passed.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& ctest --preset dev -R mirakana_rhi_tests --output-on-failure"` passed.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_d3d12_rhi_tests"` passed.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& ctest --preset dev -R mirakana_d3d12_rhi_tests --output-on-failure"` passed.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset desktop-runtime --target mirakana_runtime_host_sdl3_tests"` passed.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& ctest --preset desktop-runtime -R mirakana_runtime_host_sdl3_tests --output-on-failure"` passed.
- FIX: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` initially failed because `ThrowingSubmitRhiDevice`, `ThrowingTransitionRhiDevice`, and `ResizeRecordingRhiDevice` test fakes did not yet forward the new `IRhiDevice::wait_for_queue` pure virtual API.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_renderer_tests"` passed.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_runtime_host_tests"` passed.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& ctest --preset dev -R mirakana_renderer_tests --output-on-failure"` passed.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& ctest --preset dev -R mirakana_runtime_host_tests --output-on-failure"` passed.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -MaxFiles 1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed after selecting `generated-3d-compute-morph-queue-sync-package-smoke-d3d12-v1` as the next active plan.
