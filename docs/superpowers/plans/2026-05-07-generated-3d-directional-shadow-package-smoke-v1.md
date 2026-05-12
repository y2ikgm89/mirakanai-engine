# Generated 3D Directional Shadow Package Smoke v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a narrow generated `DesktopRuntime3DPackage` package smoke path that proves the selected generated 3D scene can request host-owned directional shadow rendering and fixed 3x3 PCF filtering.

**Architecture:** Reuse the existing SDL3 presentation directional-shadow contract already proven by `sample_desktop_runtime_game`. The generated 3D package executable and template will accept `--require-directional-shadow` and `--require-directional-shadow-filtering`, load generated shadow receiver and shadow map shaders, set `SdlDesktopPresentation*SceneRendererDesc::enable_directional_shadow_smoke`, and emit the existing `directional_shadow_*` and renderer-quality fields.

**Tech Stack:** C++23, `SdlDesktopGameHost`, `SdlDesktopPresentation*SceneRendererDesc`, HLSL/DXIL/SPIR-V package shader artifacts, PowerShell installed package validation, CMake registered desktop runtime game metadata.

---

## Context

- Master plan: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`.
- Previous generated 3D package slices added committed/generated package files, transform/morph/quaternion package smokes, D3D12/Vulkan compute morph package smokes, selected safe-point package streaming, scene GPU + postprocess + depth-aware renderer quality counters, and `playable_3d_*` aggregate counters.
- `sample_desktop_runtime_game` already proves package-visible directional shadow filtering with `directional_shadow_status=ready`, `directional_shadow_ready=1`, `directional_shadow_filter_mode=fixed_pcf_3x3`, `directional_shadow_filter_taps=9`, `directional_shadow_filter_radius_texels=1`, and `framegraph_passes=3`.
- `SdlDesktopPresentation*SceneRendererDesc` intentionally rejects directional shadow smoke when morph or compute-morph mesh bindings are simultaneously selected. This slice must keep the existing generated 3D compute/playable default package smoke intact and add a separate selected shadow smoke recipe over the same installed package artifacts.

## Constraints

- Keep shadow rendering host-owned behind `SdlDesktopPresentation*SceneRendererDesc`; do not expose native GPU/RHI handles to gameplay.
- Do not combine directional shadow smoke with generated morph, compute morph, compute-morph skin, or async telemetry smokes in the same selected run.
- Do not claim hardware comparison samplers, production shadow quality, editor shadow authoring, Metal shadow presentation, GPU timestamps, backend-native stats, broad renderer quality, or broad generated 3D production readiness.
- Update generated template output and committed sample together.
- Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` before completion reporting and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 && pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` before the slice-closing commit.

## Done When

- `sample_generated_desktop_runtime_3d_package --smoke ... --require-directional-shadow --require-directional-shadow-filtering --require-renderer-quality-gates` reports:
  - `postprocess_status=ready`
  - `postprocess_depth_input_ready=1`
  - `directional_shadow_status=ready`
  - `directional_shadow_ready=1`
  - `directional_shadow_filter_mode=fixed_pcf_3x3`
  - `directional_shadow_filter_taps=9`
  - `directional_shadow_filter_radius_texels=1`
  - `framegraph_passes=3`
  - `renderer_quality_status=ready`
  - `renderer_quality_ready=1`
  - `renderer_quality_expected_framegraph_passes=3`
  - `renderer_quality_directional_shadow_ready=1`
  - `renderer_quality_directional_shadow_filter_ready=1`
- `tools/validate-installed-desktop-runtime.ps1` accepts selected generated 3D shadow smoke args with `--require-directional-shadow` and `--require-directional-shadow-filtering`.
- The existing generated 3D default package smoke still proves the compute/playable lane without shadow flags.
- `tools/new-game.ps1 -Template DesktopRuntime3DPackage` emits the same option parsing, shader sources/artifacts, descriptor wiring, status fields, manifest recipe args, and README text.
- `games/sample_generated_desktop_runtime_3d_package/game.agent.json`, `games/CMakeLists.txt`, docs, static checks, and skills distinguish this generated 3D directional-shadow smoke from morph/compute composition, broad renderer quality, Metal parity, and broad generated 3D readiness.
- Focused build/smoke validation, strict Vulkan shadow smoke where toolchain/runtime are ready, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass or record concrete host/tool blockers.

## Tasks

### Task 1: RED

- [x] Run the committed generated 3D package smoke with `--require-directional-shadow --require-directional-shadow-filtering` before implementation.
- [x] Record the expected failure: the arguments are unknown, shadow shaders are absent, or required directional-shadow fields are absent.

### Task 2: Implement Committed Sample

- [x] Add generated 3D options and parsing for `--require-directional-shadow` and `--require-directional-shadow-filtering`, implying scene GPU, postprocess, and postprocess depth input.
- [x] Add generated 3D shadow receiver and shadow map shader paths/loaders for D3D12 and Vulkan.
- [x] Wire shadow receiver bytecode, shadow map bytecode, and `enable_directional_shadow_smoke` into D3D12/Vulkan scene renderer descriptors without enabling morph/compute bindings on shadow runs.
- [x] Emit direct `directional_shadow_*` fields and fail early when required shadow/filtering evidence is unavailable.
- [x] Update smoke assertions and renderer quality requirements for `framegraph_passes=3` when shadow is requested.
- [x] Make the committed generated scene's directional light shadow-casting so the existing scene shadow planner can produce a valid shadow plan.

### Task 3: Shader Artifacts and Metadata

- [x] Extend generated 3D scene shader artifacts with `ps_shadow_receiver`, skinned shadow receiver if needed, and `runtime_shadow.hlsl` `vs_shadow`/`ps_shadow` outputs.
- [x] Install/check those D3D12 and Vulkan artifacts for the selected package target while preserving the existing default package smoke args.
- [x] Add selected generated 3D D3D12/Vulkan directional shadow validation recipes.

### Task 4: Generated Template

- [x] Update `tools/new-game.ps1` so generated `DesktopRuntime3DPackage` games contain the same scene data, options, shader files, CMake artifact metadata, status output, README text, and manifest validation recipes.
- [x] Update `tools/check-ai-integration.ps1` to require generated 3D directional-shadow package smoke markers for committed and generated 3D package scaffolds.

### Task 5: Docs and Static Checks

- [x] Update `docs/current-capabilities.md`, `docs/roadmap.md`, this registry, the master plan, `engine/agent/manifest.json`, and relevant skills.
- [x] Keep unsupported claims explicit for Metal shadow presentation, production shadow quality, morph/compute shadow composition, hardware comparison samplers, and broad renderer quality.

### Task 6: Validate and Commit

- [x] Run focused build/test and package smoke.
- [x] Run a strict Vulkan package smoke with generated 3D directional shadows if the host/toolchain lane is ready.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` before the slice-closing commit.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED generated 3D package smoke with `--require-directional-shadow --require-directional-shadow-filtering` | FAIL as expected | `out/build/desktop-runtime/.../sample_generated_desktop_runtime_3d_package.exe --smoke ... --require-directional-shadow --require-directional-shadow-filtering --require-renderer-quality-gates` exited 1 with `unknown argument: --require-directional-shadow`. |
| `cmake --build --preset desktop-runtime --target sample_generated_desktop_runtime_3d_package --config Debug` | PASS | Rebuilt `sample_generated_desktop_runtime_3d_package.exe` after the sample/template synchronization. |
| Source-tree D3D12 generated 3D directional shadow smoke | PASS | Exit 0 with `renderer=d3d12`, `postprocess_status=ready`, `postprocess_depth_input_ready=1`, `directional_shadow_status=ready`, `directional_shadow_ready=1`, `directional_shadow_filter_mode=fixed_pcf_3x3`, `directional_shadow_filter_taps=9`, `framegraph_passes=3`, `renderer_quality_status=ready`, and `renderer_quality_expected_framegraph_passes=3`. |
| `tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package -SmokeArgs <d3d12 shadow smoke args>` | PASS | Installed package validation passed with `installed-desktop-runtime-validation: ok`, `renderer=d3d12`, `directional_shadow_ready=1`, and `desktop-runtime-package: ok (sample_generated_desktop_runtime_3d_package)`. |
| `tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package -RequireVulkanShaders -SmokeArgs <vulkan shadow smoke args>` | PASS | Installed package validation passed with `renderer=vulkan`, `directional_shadow_ready=1`, `directional_shadow_filter_mode=fixed_pcf_3x3`, `renderer_quality_directional_shadow_filter_ready=1`, and `desktop-runtime-package: ok (sample_generated_desktop_runtime_3d_package)`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `ai-integration-check: ok`; generated `DesktopRuntime3DPackage` scaffold emits the directional-shadow markers. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | `production-readiness-audit-check: ok`; unsupported gaps remain explicit. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | `format-check: ok` after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` fixed the sample main clang-format delta. |
| `git diff --check` | PASS | No whitespace errors; Git reported only line-ending conversion warnings for modified text files. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | `validate: ok`; CTest reported `100% tests passed, 0 tests failed out of 29`. Metal/Apple checks remained diagnostic/host-gated on Windows. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Dev preset configured and built successfully with MSBuild. |
