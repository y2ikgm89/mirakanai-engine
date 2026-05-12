# Generated 3D Shadow Morph Composition Package Smoke v1 Implementation Plan (2026-05-09)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `generated-3d-shadow-morph-composition-package-smoke-v1`
**Status:** Completed.
**Goal:** Add a narrow generated `DesktopRuntime3DPackage` package smoke that proves selected graphics morph mesh scene draws can compose with host-owned directional shadow receiver rendering and fixed 3x3 PCF filtering.

**Architecture:** Keep the runtime boundary host-owned. `games/sample_generated_desktop_runtime_3d_package` will select this as an explicit separate smoke recipe instead of widening the default compute/playable package lane. The renderer change stays inside first-party RHI renderer/presentation contracts by allowing `RhiDirectionalShadowSmokeFrameRenderer` to use an optional morph scene pipeline and descriptor set while keeping native handles hidden.

**Tech Stack:** C++23, `RhiDirectionalShadowSmokeFrameRenderer`, `SdlDesktopPresentation*SceneRendererDesc`, generated 3D package DXIL/SPIR-V artifacts, PowerShell package validation, CMake desktop runtime metadata.

---

## Context

- Master plan: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`.
- Generated 3D package lanes already prove CPU morph package rows, graphics morph scene GPU counters, compute morph D3D12/Vulkan package smokes, playable package aggregation, renderer quality gates, postprocess depth input, and selected directional shadow filtering.
- The current selected directional-shadow run intentionally rejects `--require-morph-package`, `--require-compute-morph`, and `--require-playable-3d-slice`. This slice narrows only the graphics morph + directional shadow receiver case behind a new explicit smoke flag.
- Existing broad shadow quality, morph-deformed shadow-caster silhouettes, compute morph + shadow, skin + morph + shadow, Metal parity, backend-native stats, GPU timestamps, and production renderer quality remain out of scope.

## Constraints

- Do not expose native OS/GPU/RHI handles to gameplay.
- Do not fold this into the default generated 3D compute/playable package smoke.
- Do not claim broad production shadow quality or full animation deformation composition; this selected smoke proves graphics morph scene-pass drawing with directional shadow receiver/postprocess counters.
- Preserve the existing compute morph, compute morph skin, async telemetry, playable 3D, Vulkan toolchain-gated, and directional shadow-only package recipes.
- Run focused renderer/sample tests before broad validation. Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` before completion reporting and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 && pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` before the slice-closing commit.

## Done When

- `RhiDirectionalShadowSmokeFrameRenderer` accepts a morph scene pipeline and records morphed scene draws with `gpu_morph_draws` and `morph_descriptor_binds` while still executing the three-pass shadow receiver frame graph.
- `SdlDesktopPresentationD3d12SceneRendererDesc` and `SdlDesktopPresentationVulkanSceneRendererDesc` allow non-skinned graphics morph bindings when directional shadow smoke is selected and bind the shadow receiver descriptor set at the correct index after morph descriptors.
- `sample_generated_desktop_runtime_3d_package` supports a new explicit `--require-shadow-morph-composition` smoke selector that implies scene GPU, morph package, postprocess depth input, directional shadow filtering, and renderer quality gates.
- The selected D3D12 installed package smoke reports:
  - `directional_shadow_status=ready`
  - `directional_shadow_ready=1`
  - `directional_shadow_filter_mode=fixed_pcf_3x3`
  - `framegraph_passes=3`
  - `scene_gpu_morph_mesh_bindings>=1`
  - `scene_gpu_morph_mesh_resolved=<max_frames>`
  - `gpu_morph_draws=<max_frames>`
  - `morph_descriptor_binds=<max_frames>`
  - `renderer_quality_status=ready`
- Manifest, sample README, generated template, docs, plan registry, and static checks distinguish this selected smoke from compute morph + shadow, morph shadow-caster deformation, Metal readiness, and broad renderer quality.
- Focused unit tests, focused sample/package smoke, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `git diff --check`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass or record concrete host/tool blockers.

## Tasks

### Task 1: RED

- [x] Add or run a renderer/sample gate that currently fails for directional shadow + graphics morph composition.
- [x] Record the expected failure before implementation.

### Task 2: Renderer And Presentation

- [x] Add optional morph scene pipeline support to `RhiDirectionalShadowSmokeFrameRenderer`.
- [x] Allow non-skinned graphics morph bindings with directional shadow in SDL3 presentation validation.
- [x] Bind the shadow receiver descriptor set after morph/skinning descriptor layouts using the actual pipeline layout ordering.

### Task 3: Generated 3D Sample And Package Recipes

- [x] Add `--require-shadow-morph-composition` parsing and smoke assertions.
- [x] Wire morph vertex shader/assets/bindings when the selected composition smoke is enabled.
- [x] Add committed sample manifest/README/validation recipe metadata and CMake package smoke coverage.
- [x] Update `tools/new-game.ps1` so new `DesktopRuntime3DPackage` scaffolds emit the same selected recipe and wording.

### Task 4: Docs, Manifest, And Static Checks

- [x] Update the plan registry, master plan, `engine/agent/manifest.json`, current docs, sample manifests, and generated-game validation docs.
- [x] Add static checks for the new smoke flag and explicit unsupported boundaries.

### Task 5: Validate And Commit

- [x] Run focused renderer/runtime host tests.
- [x] Run focused generated 3D package build/smoke.
- [x] Run package smoke for the selected D3D12 shadow+morph recipe.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Stage only this slice and commit.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `cmake --build --preset dev --target mirakana_renderer_tests --config Debug` before implementation | Expected FAIL | RED: `scene_morph_graphics_pipeline` was missing from `RhiDirectionalShadowSmokeFrameRendererDesc`. |
| `cmake --build --preset dev --target mirakana_renderer_tests --config Debug` | PASS | Renderer test target rebuilt after morph scene pipeline and material-binding validation changes. |
| `ctest --preset dev --output-on-failure -R "^mirakana_renderer_tests$"` | PASS | 1/1 `mirakana_renderer_tests` passed, including directional shadow morph draw and missing-material negative coverage. |
| `cmake --build --preset desktop-runtime --target sample_generated_desktop_runtime_3d_package --config Debug` | PASS | Generated 3D package sample rebuilt with shifted shadow receiver shader artifacts. |
| `out\build\desktop-runtime\games\Debug\sample_generated_desktop_runtime_3d_package\sample_generated_desktop_runtime_3d_package.exe --smoke --require-config runtime/sample_generated_desktop_runtime_3d_package.config --require-scene-package runtime/sample_generated_desktop_runtime_3d_package.geindex --require-d3d12-scene-shaders --video-driver windows --require-d3d12-renderer --require-shadow-morph-composition` | PASS | Reported `renderer=d3d12`, `scene_gpu_morph_mesh_bindings=1`, `scene_gpu_morph_mesh_resolved=2`, `renderer_gpu_morph_draws=2`, `renderer_morph_descriptor_binds=2`, `directional_shadow_ready=1`, `directional_shadow_filter_mode=fixed_pcf_3x3`, and `framegraph_passes=3`. |
| `tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package -SmokeArgs @('--smoke', '--require-config', 'runtime/sample_generated_desktop_runtime_3d_package.config', '--require-scene-package', 'runtime/sample_generated_desktop_runtime_3d_package.geindex', '--require-d3d12-scene-shaders', '--video-driver', 'windows', '--require-d3d12-renderer', '--require-shadow-morph-composition')` | PASS | Installed package validation succeeded with the same positive morph/shadow counters, installed consumer validation, and CPack ZIP generation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | `json-contract-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `ai-integration-check: ok`; generated scaffold dry runs completed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | `production-readiness-audit-check: ok`; 3D gap remains `implemented-generated-desktop-3d-package-proof`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Initial clang-format miss was corrected with `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`; rerun passed. |
| `git diff --check` | PASS | Exit 0; only line-ending normalization warnings were printed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full validation passed: schema/agent/static checks, production-readiness audit, toolchain checks, tidy check, dev configure/build, and 29/29 dev CTest tests. Metal/Apple diagnostics remain host-gated as expected on Windows. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Dev preset configured and built successfully. |
