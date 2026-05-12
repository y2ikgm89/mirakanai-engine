# Generated 3D Compute Morph Async Telemetry Package Smoke D3D12 v1 (2026-05-06)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `generated-3d-compute-morph-async-telemetry-package-smoke-d3d12-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Promote the completed RHI compute-morph async telemetry into the generated `DesktopRuntime3DPackage` D3D12 smoke path
so package validation can observe first-party compute submit, graphics queue-wait, and graphics submit sequencing
without exposing queue/fence handles, native backend objects, GPU timestamps, or performance overlap claims.

## Architecture

Keep ownership in the SDL3 desktop runtime host and RHI adapters. Generated gameplay may request the smoke and read
first-party counters/diagnostics only; it must not receive `IRhiDevice`, command lists, descriptor handles, queue/fence
handles, swapchain frames, native D3D12 objects, or backend stats.

## Tech Stack

C++23, `mirakana_runtime_host_sdl3_presentation`, `mirakana_rhi`, `mirakana_rhi_d3d12`, generated `DesktopRuntime3DPackage` scaffolding,
D3D12 focused runtime-host/package tests, static docs checks.

---

## Context

- Runtime RHI Compute Morph Async Telemetry D3D12 v1 added backend-neutral `RhiStats` queue-submit and queue-wait fence
  sequencing fields plus D3D12 private context coverage.
- Generated D3D12 compute morph package smokes already report queue-wait counts for POSITION, NORMAL/TANGENT, and
  skin+compute paths.
- Package validation still lacks a generated smoke requirement that ties those package counters to the new sequencing
  telemetry.

## Constraints

- Do not expose native queue/fence handles, command lists, descriptor handles, swapchain frames, backend stats, GPU
  timestamps, or D3D12 objects to generated gameplay.
- Do not claim async overlap, GPU performance improvement, Vulkan/Metal parity, frame graph scheduling, graphics
  morph+skin composition, directional-shadow morph rendering, scene-schema compute-morph authoring, or broad renderer
  quality.
- Preserve existing generated compute morph POSITION, NORMAL/TANGENT, queue-wait, and skin+compute package smokes.
- Keep package-visible output deterministic and first-party.

## Done When

- A RED generated/package or runtime-host test fails first because generated D3D12 compute morph package smoke cannot
  require async sequencing telemetry.
- Generated `DesktopRuntime3DPackage` D3D12 package smoke can report and require first-party sequencing telemetry for
  the host-owned compute morph path.
- Docs, manifest, plan registry, static checks, and skills describe this as generated D3D12 package telemetry only.
- Focused tests plus `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, validate-scoped tidy, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`
  pass, or concrete host/tool blockers are recorded.

## Tasks

- [x] Inspect existing generated compute morph package output and SDL3 presentation stats propagation.
- [x] Add a RED package/runtime-host test for missing generated async sequencing telemetry.
- [x] Implement minimal package-visible first-party counters/diagnostics.
- [x] Update generated scaffold validation requirements without exposing native handles or backend stats.
- [x] Update docs, manifest, plan registry, static checks, skills/subagents, and validation evidence.

## Validation Evidence

- RED: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset desktop-runtime --target mirakana_runtime_host_sdl3_public_api_compile"` failed with missing `SdlDesktopPresentationSceneGpuBindingStats` async telemetry members after the public API compile probe referenced them.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed because the generated `DesktopRuntime3DPackage` installed D3D12 recipe lacked `--require-compute-morph-async-telemetry`.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset desktop-runtime --target mirakana_runtime_host_sdl3_public_api_compile"` passed after adding package-visible first-party async telemetry fields.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset desktop-runtime --target mirakana_runtime_host_sdl3_tests"` passed after wiring runtime-host stats to `IRhiDevice::stats()`.
- GREEN: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& ctest --preset desktop-runtime -R mirakana_runtime_host_sdl3_tests --output-on-failure"` passed.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after the generated scaffold emitted `--require-compute-morph-async-telemetry` and `scene_gpu_compute_morph_async_*` output fields.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed after running `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` to apply clang-format to `scene_gpu_binding_injecting_renderer.hpp`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed after the public SDL3 runtime-host stats additions.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -MaxFiles 1` passed with the generated compile database for the default `dev` preset.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed, including agent/config/json/dependency/toolchain/static checks, public API boundary check, tidy smoke, MSVC dev build, and 29/29 CTest tests. Metal/Apple diagnostics remained explicit host/tooling blockers only and did not fail validation.
