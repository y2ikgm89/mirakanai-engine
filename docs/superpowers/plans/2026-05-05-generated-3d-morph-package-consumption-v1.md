# Generated 3D Morph Package Consumption v1 Implementation Plan (2026-05-05)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extend generated `DesktopRuntime3DPackage` games so their package smoke consumes a cooked `morph_mesh_cpu` payload plus a cooked morph-weight `animation_float_clip` and reports deterministic CPU morph evaluation counters.

**Architecture:** Keep morph byte payload validation in `mirakana_runtime`, scalar clip sampling in `mirakana_animation`, and generated-game package orchestration in `tools/new-game.ps1`. Add a narrow `mirakana_scene_renderer` helper that bridges runtime morph payload rows to `mirakana_animation` CPU morph evaluation for smoke telemetry only; do not claim GPU morph deformation or renderer-visible morphed geometry.

**Tech Stack:** C++23, `mirakana_scene_renderer`, `mirakana_runtime`, `mirakana_animation`, `tools/new-game.ps1`, `mirakana_scene_renderer_tests`, `tools/check-ai-integration.ps1`.

---

**Plan ID:** `generated-3d-morph-package-consumption-v1`  
**Status:** Completed  
**Date:** 2026-05-05  
**Owner:** Codex  

## Context

- `morph_mesh_cpu` package rows, `RuntimeMorphMeshCpuPayload`, CPU morph POSITION/NORMAL/TANGENT evaluation, and morph weight float-clip bridge rows already exist.
- Generated `DesktopRuntime3DPackage` games currently ship cooked texture/mesh/material/scene and a scalar transform animation clip.
- The remaining generated-game morph gap needs package consumption evidence before any GPU morph or renderer-visible deformation slice.

## Constraints

- Do not add a new runtime asset kind or third-party dependency.
- Do not make `mirakana_runtime` depend on `mirakana_animation`, or generated games depend on `mirakana_tools`.
- Do not claim GPU morph, runtime vertex upload, renderer-visible morphed geometry, broad authored animation workflow, or broad generated 3D production readiness.
- Keep source authoring files out of `runtimePackageFiles`.
- Preserve host gates: source-tree smoke must remain `NullRenderer`/dummy-video capable; D3D12/Vulkan package smokes remain host/toolchain-gated.
- Add tests before production behavior and run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` because a public helper is added.

## Done When

- `mirakana_scene_renderer` exposes and tests a helper that samples an `AnimationFloatClipSourceDocument`, applies sampled `gltf/node/<n>/weights/<i>` rows to a `RuntimeMorphMeshCpuPayload`, and returns deterministic morphed positions and counters.
- `DesktopRuntime3DPackage` generated games include a cooked `morph_mesh_cpu` asset, a cooked morph-weight `animation_float_clip`, and source authoring rows for both without shipping source files in `runtimePackageFiles`.
- Generated source-tree and package smoke arguments can require morph package consumption, and smoke output includes deterministic morph counters.
- Static AI integration checks validate the generated runtime files, manifest entries, CMake smoke args, generated `main.cpp` wiring, and `.geindex` package rows.
- Docs, roadmap/master plan, plan registry, manifest, and capability guidance describe the narrow capability and its boundaries.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes or records a concrete host/tool blocker.

## Tasks

- [x] Add RED `mirakana_scene_renderer_tests` coverage for successful runtime morph payload + float clip sampling and invalid target-prefix failure.
- [x] Add public scene-renderer morph sampling result type and helper declaration.
- [x] Implement little-endian morph payload decode, sampled weight binding, CPU position morph evaluation, and deterministic diagnostics in `mirakana_scene_renderer`.
- [x] Update `DesktopRuntime3DPackage` generated cooked package files with deterministic `morph_mesh_cpu` and morph-weight `animation_float_clip` records.
- [x] Update generated 3D main/CMake/manifest smoke wiring for `--require-morph-package`.
- [x] Update static AI integration checks for the new generated scaffold contract.
- [x] Update docs, roadmap/master plan, manifest, and this plan with validation evidence.
- [x] Run focused tests, formatting, API boundary, agent checks, and full validation.

## Validation Evidence

| Check | Result | Notes |
| --- | --- | --- |
| RED focused build/test | Passed | Initial focused build failed as expected before the helper existed: `sample_runtime_morph_mesh_cpu_animation_float_clip` was not a `ge` member. |
| Focused `mirakana_scene_renderer_tests` | Passed | Sanitized MSBuild for `mirakana_scene_renderer_tests.vcxproj` succeeded with 0 warnings/0 errors; `out\build\dev\Debug\mirakana_scene_renderer_tests.exe` passed 29 tests including the new morph package sampling cases. |
| Scaffold/static check | Passed | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` generated `DesktopRuntime3DPackage` and reported `ai-integration-check: ok`, including `--require-morph-package`, cooked/source morph files, manifest rows, CMake args, and `.geindex` rows. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed | Passed after targeted `clang-format` on changed scene-renderer test/source/header files. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed | `public-api-boundary-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | `validate: ok`; 29/29 CTest tests passed. Metal and Apple checks remain diagnostic-only host gates on this Windows host. |
