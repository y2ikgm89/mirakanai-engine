# Desktop Runtime Generated Package Authoring Hardening Implementation Plan (2026-05-01)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Harden the generated desktop runtime package path so AI/user-created desktop runtime games from `tools/new-game.ps1` have an explicit source-tree, installed-package, manifest-derived package-file, cooked-scene, and shader-artifact validation lane.

**Architecture:** Keep this slice on the existing `mirakana::SdlDesktopGameHost`, `MK_add_desktop_runtime_game`, `PACKAGE_FILES_FROM_MANIFEST`, and metadata-driven package scripts. Do not add renderer/RHI behavior unless validation proves a targeted gap; generated gameplay remains on public `mirakana::` contracts and native SDL3/GPU handles stay inside host/backend adapters.

**Tech Stack:** C++23, CMake, CTest, PowerShell 7 validation scripts under `tools/`, SDL3 desktop runtime host, generated `DesktopRuntimePackage`, `DesktopRuntimeCookedScenePackage`, and `DesktopRuntimeMaterialShaderPackage` samples, D3D12 DXIL shader artifacts, host-gated Vulkan SPIR-V shader artifacts, `engine/agent/manifest.json`, docs/roadmap, and game manifests.

---

## Goal

Make the generated desktop runtime package path as repeatable as the hand-curated sample lane: a generated config-only game, generated cooked-scene game, and generated material/shader game can be understood from manifests, validated from the source tree, packaged with manifest-derived runtime files, and smoke-tested after install through metadata-selected package args.

## Context

- `core-first-mvp` is closed by `docs/superpowers/plans/2026-05-01-core-first-mvp-closure.md`.
- `docs/superpowers/plans/2026-05-01-desktop-runtime-productization.md` is complete and established `sample_desktop_runtime_game` as the representative non-shell package proof.
- `tools/new-game.ps1` already supports `DesktopRuntimePackage`, `DesktopRuntimeCookedScenePackage`, and `DesktopRuntimeMaterialShaderPackage`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` already requires the three generated sample targets to be registered and validates their manifest-derived package-file metadata.
- `tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_package` and `tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_cooked_scene_package` are documented. The material/shader generated package lane existed in manifest recipes and now has an explicit source-tree shader-artifact smoke plus docs/manifest synchronization.

## Constraints

- Do not append to the completed `core-first-mvp` or `desktop-runtime-productization` plans.
- Do not introduce third-party dependencies.
- Do not expose native OS, SDL3, D3D12, Vulkan, Metal, Dear ImGui, or RHI handles through public game APIs.
- Keep `engine/core` independent from platform, renderer, asset format, editor, SDL3, OS, GPU, and Dear ImGui code.
- Keep source material and HLSL authoring files out of `game.agent.json.runtimePackageFiles`; only runtime config, `.geindex`, and cooked payloads are package files.
- Treat Vulkan shader/package validation as host/toolchain-gated unless the exact command passes.

## Done When

- [x] `docs/superpowers/plans/README.md` marks this plan as the active slice while work is underway and completed after verification.
- [x] Baseline `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1` have been run, with sandbox-only package blockers classified if they occur.
- [x] A representative generated material/shader package target has a source-tree shader-artifact smoke when DXC is available, without requiring a real D3D12/Vulkan window in the default source-tree lane.
- [x] `tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_material_shader_package` validates installed config, cooked scene package files, D3D12 shader artifacts, scene GPU bindings, and postprocess status on this host, or records a concrete host/toolchain blocker.
- [x] `docs/testing.md`, `docs/workflows.md`, `docs/roadmap.md`, `docs/ai-game-development.md`, and `engine/agent/manifest.json` honestly describe the generated package validation path and remaining host-gated work.
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` passes.
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1` passes.
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes.
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` passes.
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` pass after manifest/docs/tooling changes.
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` is run if public headers or backend interop surfaces change.

## Implementation Tasks

### Task 1: Baseline And Gap Confirmation

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_material_shader_package`.
- [x] Record baseline results and any sandbox-only vcpkg extraction blockers.
- [x] Confirm whether `sample_generated_desktop_runtime_material_shader_package_shader_artifacts_smoke` is absent from the source-tree CTest list before implementation.

### Task 2: Source-Tree Generated Shader Smoke

- [x] Add a focused source-tree CTest smoke for generated material/shader package targets when DXC has produced D3D12 scene shader artifacts.
- [x] Ensure the executable target depends on its shader-artifact target when that smoke is enabled.
- [x] Keep the source-tree smoke limited to shader/package-file availability and cooked-scene loading; do not require real D3D12/Vulkan renderer presentation in the default `desktop-game-runtime` lane.
- [x] Run the focused desktop-runtime configure/build/CTest path and confirm the new smoke is exercised when available.

### Task 3: Documentation And Agent Contract Sync

- [x] Update generated desktop runtime package instructions in `docs/testing.md`, `docs/workflows.md`, `docs/ai-game-development.md`, and `docs/roadmap.md`.
- [x] Update `engine/agent/manifest.json` validation recipes/readiness notes so generated config-only, cooked-scene, and material/shader package lanes are visible and scoped correctly.
- [x] Keep host-gated Vulkan and future Metal/material-graph/live-shader work explicit.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.

### Task 4: Final Validation And Closure

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_material_shader_package`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` if public headers/backend interop changed.
- [x] Record final validation evidence in this plan.
- [x] Mark this plan completed in the registry.

## Validation Evidence

- Baseline before implementation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` PASS, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` PASS, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` PASS, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1` PASS after sandbox-only vcpkg 7zip `CreateFileW stdin failed with 5` was rerun with approval, and `tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_material_shader_package` PASS after the same sandbox-only rerun.
- RED check: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed while `tools/check-ai-integration.ps1` required `${MK_SCENE_SHADER_TARGET}_shader_artifacts_smoke` in `games/CMakeLists.txt`.
- Implementation: `games/CMakeLists.txt` now adds `${MK_SCENE_SHADER_TARGET}_shader_artifacts_smoke` when DXC-backed D3D12 scene/postprocess artifacts are configured and makes the executable target depend on its shader target. `tools/check-ai-integration.ps1` statically requires the source-tree smoke.
- GREEN checks: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` PASS; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` PASS.
- Final source-tree runtime: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` PASS, 13/13 tests including `sample_generated_desktop_runtime_material_shader_package_shader_artifacts_smoke`.
- Final default package: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1` PASS after sandbox-only vcpkg 7zip rerun with approval.
- Final generated package: `tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_material_shader_package` PASS after sandbox-only vcpkg 7zip rerun with approval; installed status reported `renderer=d3d12`, `scene_gpu_status=ready`, `postprocess_status=ready`, `framegraph_passes=2`, `scene_meshes=2`, and `scene_materials=2`.
- Final default validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` PASS with diagnostic-only Metal tool absence, Apple packaging host gate, Android signing/device smoke not configured/connected, and non-strict tidy compile database blocker unchanged from existing host state.
- Final build/API checks: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` PASS; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` PASS.
