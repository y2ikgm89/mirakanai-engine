# Generated 3D Compute Morph Queue Sync Package Smoke D3D12 v1 (2026-05-06)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `generated-3d-compute-morph-queue-sync-package-smoke-d3d12-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Add package-visible generated `DesktopRuntime3DPackage` D3D12 smoke evidence that host-owned compute morph dispatches
order graphics consumption through the public RHI queue wait contract instead of a CPU-side wait, without exposing RHI or
native backend details to generated gameplay code.
This is package-visible compute morph queue-wait smoke evidence only; it does not promote backend-native queue/fence
details into generated gameplay.
Boundary: without exposing RHI or native backend details.

## Context

- `runtime-rhi-compute-morph-queue-synchronization-d3d12-v1` added `IRhiDevice::wait_for_queue`, queue-wait stats,
  deterministic NullRHI validation, and D3D12 public-device routing through private queue/fence primitives.
- `generated-3d-compute-morph-normal-tangent-package-smoke-d3d12-v1` proves generated package POSITION/NORMAL/TANGENT
  compute morph output rendering and reports `scene_gpu_compute_morph_tangent_frame_output`.
- The generated package smoke currently reports compute morph bindings, dispatches, resolution, draws, and tangent-frame
  output, but it does not yet expose a first-party package smoke field that proves the queue wait path was used.

## Constraints

- Keep `IRhiDevice`, command lists, queue/fence handles, descriptor handles, swapchain frames, and native D3D12 handles
  private to the host/RHI layers.
- Expose only first-party smoke counters/diagnostics, not backend-native stats or raw RHI objects.
- Do not claim async compute overlap/performance, Vulkan/Metal parity, frame graph scheduling, skin+morph composition,
  directional-shadow morph rendering, scene-schema compute-morph authoring, or broad renderer quality.
- Preserve existing POSITION-only and POSITION/NORMAL/TANGENT compute morph package smoke behavior.
- Add focused RED tests/static expectations before production implementation.

## Done When

- A focused test/static expectation fails first because generated package compute morph smoke has no queue-wait evidence.
- The SDL3 D3D12 presentation path can report a deterministic first-party compute morph queue-wait smoke counter.
- Generated `DesktopRuntime3DPackage` smoke output and validation can require the queue-wait evidence when compute morph
  package smoke is requested.
- Docs, manifest, plan registry, static checks, skills, and subagents describe this as package smoke evidence only, not
  async overlap or backend parity.
- Focused tests plus `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, validate-scoped tidy, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`
  pass, or concrete host/tool blockers are recorded.

## Tasks

- [x] Inspect current SDL3 presentation report fields, `SceneGpuBindingInjectingRenderer` stats, generated package smoke
      output, and `validate-runtime-scene-package` expectations.
- [x] Add RED coverage for a generated package queue-wait smoke counter and validation expectation.
- [x] Add host-owned compute morph queue-wait smoke reporting without exposing `IRhiDevice` or native handles.
- [x] Extend generated `DesktopRuntime3DPackage` smoke output/static checks to require the queue-wait evidence.
- [x] Update docs, manifest, plan registry, static checks, skills/subagents, and validation evidence.

## Validation Evidence

- RED: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset desktop-runtime --target mirakana_runtime_host_sdl3_public_api_compile"` failed because `SdlDesktopPresentationSceneGpuBindingStats::compute_morph_queue_waits` did not exist.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed because generated `DesktopRuntime3DPackage` scaffold output did not contain `scene_gpu_compute_morph_queue_waits`.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset desktop-runtime --target mirakana_runtime_host_sdl3_public_api_compile"` passed.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset desktop-runtime --target mirakana_runtime_host_sdl3_tests"` passed.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& ctest --preset desktop-runtime -R mirakana_runtime_host_sdl3_tests --output-on-failure"` passed.
- GREEN: docs/manifest/skills sync after selecting Runtime RHI Compute Morph Skin Composition D3D12 v1 passed `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`; the first rerun exposed a `recommendedNextPlan.reason` wording mismatch, and the second rerun passed.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed after running `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` for `runtime_host_sdl3_public_api_compile.cpp`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -MaxFiles 1` passed.
- GREEN: final focused package-smoke checks passed: `mirakana_runtime_host_sdl3_public_api_compile` build, `mirakana_runtime_host_sdl3_tests` build, and `ctest --preset desktop-runtime -R mirakana_runtime_host_sdl3_tests --output-on-failure`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed after docs/manifest/static-check synchronization; diagnostic-only host blockers remained missing Metal tools and Apple packaging tools on Windows.
