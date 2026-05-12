# Renderer Resource Residency Upload Execution v1 Implementation Plan (2026-05-02)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Move the current renderer/RHI resource and upload/staging foundations from planning contracts toward a narrow validated execution path for explicitly selected cooked runtime texture, mesh, and material payloads without exposing native handles to gameplay.

**Architecture:** Keep gameplay on cooked package loading plus `mirakana_scene_renderer` submission. Host/backend adapters may own `IRhiDevice`, upload execution, residency tracking, and `SceneGpuBindingPalette` construction behind first-party contracts. Reuse `mirakana_runtime_rhi`, `mirakana_runtime_scene_rhi`, `RhiResourceLifetimeRegistry`, `RhiUploadStagingPlan`, desktop runtime package metadata, and existing D3D12/Vulkan host gates. Do not turn this into package streaming, broad renderer quality, public native handle exposure, editor productization, material/shader graph authoring, live shader generation, or Metal readiness.

**Tech Stack:** C++23, `mirakana_runtime`, `mirakana_runtime_rhi`, `mirakana_runtime_scene_rhi`, `mirakana_rhi`, `mirakana_renderer`, D3D12/Vulkan private backends, desktop runtime package validation scripts, static checks, docs, and focused tests.

---

## Goal

Prove one narrow resource execution loop:

- explicitly selected cooked texture, mesh, and material payloads load from a runtime package
- host-owned upload execution produces first-party GPU binding descriptors or clear blocked diagnostics
- residency/lifetime rows stay internal to renderer/RHI ownership
- generated/sample package validation can distinguish package/scene validation from host-gated residency/upload execution

## Context

Renderer/RHI resource foundation, upload/staging planning, runtime RHI upload helpers, and sample D3D12/Vulkan scene GPU package proofs already exist. The next uplift should make the boundary more operational for AI-generated packages without broadening gameplay APIs or claiming production renderer quality.

## Constraints

- Do not expose native API handles or `IRhiDevice` ownership to gameplay code.
- Do not implement package streaming, broad residency budgets, async streaming, or hot-reload safe-point unload in this slice.
- Do not add third-party dependencies.
- Do not claim Metal readiness from Windows validation.
- Keep Vulkan strict and toolchain-gated.
- Keep material/shader graph authoring and live shader generation out of scope.

## Done When

- A RED -> GREEN record exists in this plan.
- Focused tests or static checks distinguish package validation, scene instantiation, and host-owned upload/residency execution.
- One selected package path can report uploaded texture/mesh/material binding readiness or precise host-gated diagnostics through existing first-party report rows.
- `engine/agent/manifest.json`, docs, static checks, and validation recipes remain honest about host gates and unsupported claims.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, focused checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## Implementation Tasks

### Task 1: Inventory Resource And Upload Boundaries

- [x] Read `mirakana_runtime_rhi`, `mirakana_runtime_scene_rhi`, `mirakana_rhi`, `mirakana_renderer`, desktop runtime package metadata, and current D3D12/Vulkan package smoke checks.
- [x] Identify the smallest host-owned upload execution path that can be validated without gameplay native handles.
- [x] Record non-goals before RED checks are added.

Inventory notes:

- `mirakana_runtime_rhi` already uploads cooked texture bytes, mesh vertex/index bytes, and material factor uniform bytes through an `IRhiDevice` while preserving owner-device provenance.
- `mirakana_runtime_scene_rhi::build_runtime_scene_gpu_binding_palette` is the narrow bridge from `RuntimeAssetPackage` plus `SceneRenderPacket` into retained mesh uploads, texture uploads, material descriptor bindings, and scene-shared material pipeline layouts.
- `mirakana_runtime_host_sdl3_presentation` is the host-owned execution point: it validates requested vertex input against cooked mesh payload layout, privately creates the D3D12/Vulkan `IRhiDevice`, builds scene GPU bindings, creates the graphics pipeline from the palette material pipeline layout, and wraps the renderer in `SceneGpuBindingInjectingRenderer`.
- Existing package smokes already distinguish cooked package/scene validation from host-gated scene GPU execution through `--require-scene-package`, `--require-scene-gpu-bindings`, backend selection, and `scene_gpu_status`, but they did not report texture upload counts or uploaded byte totals.
- Non-goals for this slice: public native handles, public `IRhiDevice` access, package streaming, broad residency budgets, async upload rings, material/shader graphs, live shader generation, Metal readiness, editor resource panels, or general production renderer quality.

### Task 2: RED Checks

- [x] Add failing tests/static checks for explicit package/scene/upload boundary reporting.
- [x] Add failing checks rejecting public native handles, package streaming, broad residency budgets, material/shader graph, live shader generation, Metal/general renderer quality claims.
- [x] Record RED evidence.

### Task 3: Host-Owned Upload/Residency Execution

- [x] Implement or tighten the selected execution path.
- [x] Keep resource lifetime/residency rows behind renderer/RHI ownership.
- [x] Keep gameplay code on package loading and scene submission contracts.

Implementation notes:

- Added `mirakana::runtime_scene_rhi::execute_runtime_scene_gpu_upload` and `RuntimeSceneGpuUploadExecutionReport` as a backend-neutral host-owned report over the existing cooked texture/mesh/material upload path.
- `SdlDesktopPresentationSceneGpuBindingStats` now carries first-party upload counts and uploaded byte totals only; it does not expose `IRhiDevice`, native handles, descriptor handles, swapchain frames, GPU timestamps, or `SceneGpuBindingPalette`.
- `SceneGpuBindingInjectingRenderer` derives upload counters from the retained scene GPU binding result while continuing to inject mesh/material GPU bindings internally.
- Selected desktop runtime game status lines now print `scene_gpu_mesh_uploads`, `scene_gpu_texture_uploads`, `scene_gpu_material_uploads`, `scene_gpu_uploaded_texture_bytes`, `scene_gpu_uploaded_mesh_bytes`, and `scene_gpu_uploaded_material_factor_bytes`.
- Sample game sources no longer include `mirakana/runtime_rhi/runtime_upload.hpp` just to obtain the lit mesh stride; game code keeps the vertex-input stride as local package/shader contract data.

### Task 4: Docs Manifest Validation

- [x] Update manifest/docs/static checks.
- [x] Run required validation.
- [x] Advance the plan registry to the next focused material/shader authoring slice if this slice completes.

## Validation Evidence

Record command results here while implementing this plan.

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` failed as expected after adding runtime scene RHI upload execution tests because `mirakana::runtime_scene_rhi::RuntimeSceneGpuUploadExecutionDesc`, `RuntimeSceneGpuUploadExecutionStatus`, and `execute_runtime_scene_gpu_upload` did not exist.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` failed as expected because `engine/agent/manifest.json.aiOperableProductionLoop` was missing required `resourceExecutionLoops`.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed as expected because `docs/current-capabilities.md` did not contain `Renderer Resource Residency Upload Execution v1`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` passed after adding the host-owned upload execution API and report (`CTest 28/28 passed`).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed after adding `aiOperableProductionLoop.resourceExecutionLoops`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after syncing manifest/docs/static checks.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed after public report/header changes (`public-api-boundary-check: ok`).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe desktop-runtime-sample-game-scene-gpu-package -HostGateAcknowledgements d3d12-windows-primary` passed, validating the selected `sample_desktop_runtime_game` package lane with the new backend-neutral scene GPU upload counters.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` passed again after sample/CMake boundary cleanup (`CTest 28/28 passed`).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe desktop-runtime-sample-game-scene-gpu-package -HostGateAcknowledgements d3d12-windows-primary` passed after cleanup (`status: passed`, `durationSeconds: 73.229`).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after advancing `currentActivePlan` to Material Shader Authoring Loop v1 and creating the next package streaming/residency budget contract plan.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract` passed after docs/manifest synchronization.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed (`validate: ok`, `CTest 28/28 passed`). Diagnostic-only gates remain honest: Metal tools missing on this Windows host, Apple packaging requires macOS/Xcode, Android release signing/device smoke not fully configured, and strict tidy analysis requires a compile database.
