# Generated 3D Renderer Quality Package Smoke v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a narrow generated `DesktopRuntime3DPackage` package smoke gate that reports existing renderer quality diagnostics for selected D3D12/Vulkan scene GPU + postprocess + framegraph execution evidence.

**Architecture:** Reuse `mirakana::evaluate_sdl_desktop_presentation_quality_gate` from `mirakana_runtime_host_sdl3_presentation`. The committed and generated 3D package executable will accept `--require-renderer-quality-gates`, build a descriptor that requires scene GPU bindings and postprocess only, and emit package-visible `renderer_quality_*` fields without requiring depth input, directional shadows, GPU timestamps, native handles, or general renderer quality.

**Tech Stack:** C++23, `mirakana_runtime_host_sdl3_presentation`, generated `DesktopRuntime3DPackage`, PowerShell installed package validation, CMake registered desktop runtime game metadata.

---

## Context

- Master plan: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`.
- Existing renderer quality gate API: `mirakana::evaluate_sdl_desktop_presentation_quality_gate`.
- Existing generated 3D sample: `games/sample_generated_desktop_runtime_3d_package`.
- Existing `sample_desktop_runtime_game` quality gate covers scene GPU + postprocess + depth input + directional shadow with `framegraph_passes=3`.
- This slice is intentionally narrower: generated 3D package proof currently has selected scene GPU + postprocess with `framegraph_passes=2`.

## Constraints

- Keep generated gameplay on public `mirakana::` APIs and existing host-owned SDL3 presentation contracts.
- Do not add new renderer/RHI APIs or expose native/RHI handles.
- Do not claim depth-aware postprocess, directional shadows, Metal readiness, cross-backend performance parity, GPU timestamps, backend-native stats, broad renderer quality, production material graph/live shader generation, or broad generated 3D readiness.
- Update generated template output and committed sample together.
- Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` before completion reporting.

## Done When

- `sample_generated_desktop_runtime_3d_package --smoke ... --require-renderer-quality-gates` reports:
  - `renderer_quality_status=ready`
  - `renderer_quality_ready=1`
  - `renderer_quality_diagnostics=0`
  - `renderer_quality_expected_framegraph_passes=2`
  - `renderer_quality_framegraph_passes_ok=1`
  - `renderer_quality_framegraph_execution_budget_ok=1`
  - `renderer_quality_scene_gpu_ready=1`
  - `renderer_quality_postprocess_ready=1`
- `tools/validate-installed-desktop-runtime.ps1` validates generated 3D quality fields without requiring depth/shadow fields when smoke args omit those requirements.
- `tools/new-game.ps1 -Template DesktopRuntime3DPackage` emits the same option, counters, manifest recipe args, and docs text.
- `games/sample_generated_desktop_runtime_3d_package/game.agent.json`, `games/CMakeLists.txt`, docs, static checks, and skills distinguish this generated 3D quality smoke from broad renderer quality.
- Focused build/smoke validation, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete host/tool blockers.

## Tasks

### Task 1: RED

- [x] Run the committed generated 3D package smoke with `--require-renderer-quality-gates` before implementation.
- [x] Record the expected failure: the argument is unknown or required `renderer_quality_*` fields are absent.

### Task 2: Implement Committed Sample

- [x] Add `DesktopRuntimeOptions::require_renderer_quality_gates` and parse `--require-renderer-quality-gates`.
- [x] Build a generated-3D-specific `SdlDesktopPresentationQualityGateDesc` requiring scene GPU bindings and postprocess only.
- [x] Emit `renderer_quality_*` fields on the existing status line and fail smoke when the required generated-3D gate is not ready.

### Task 3: Validation Script and Metadata

- [x] Generalize `tools/validate-installed-desktop-runtime.ps1` quality checks so generated 3D smokes can require scene GPU + postprocess + framegraph=2 without depth/shadow.
- [x] Add the requirement to `games/CMakeLists.txt` installed package smoke args for `sample_generated_desktop_runtime_3d_package`.
- [x] Update `games/sample_generated_desktop_runtime_3d_package/game.agent.json` validation recipes and manifest text.

### Task 4: Generated Template

- [x] Update `tools/new-game.ps1` so generated `DesktopRuntime3DPackage` games contain the same option, counters, recipe args, manifest fields, and README text.

### Task 5: Docs and Static Checks

- [x] Update `docs/current-capabilities.md`, `docs/roadmap.md`, this registry, the master plan, `engine/agent/manifest.json`, and relevant skills.
- [x] Update `tools/check-ai-integration.ps1` to require the generated 3D renderer quality markers for committed and generated 3D package scaffolds.

### Task 6: Validate and Commit

- [x] Run focused build/test and package smoke.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` before the slice-closing commit.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED generated 3D package smoke with `--require-renderer-quality-gates` | FAIL as expected | `out/build/desktop-runtime/.../sample_generated_desktop_runtime_3d_package.exe --smoke ... --require-renderer-quality-gates` exited 2 with `unknown argument: --require-renderer-quality-gates`. |
| Generated `DesktopRuntime3DPackage` scaffold | PASS | `tools/new-game.ps1 -Template DesktopRuntime3DPackage` emitted `--require-renderer-quality-gates`, `evaluate_sdl_desktop_presentation_quality_gate`, and `renderer_quality_expected_framegraph_passes=2` markers. |
| Focused build/test | PASS | `cmake --build --preset desktop-runtime --target sample_generated_desktop_runtime_3d_package` passed. `ctest --preset desktop-runtime --output-on-failure -R sample_generated_desktop_runtime_3d_package` passed 2/2 tests. |
| Installed D3D12 package smoke | PASS | `tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package` passed and installed validation reported `renderer_quality_status=ready`, `renderer_quality_ready=1`, `renderer_quality_diagnostics=0`, `renderer_quality_expected_framegraph_passes=2`, `renderer_quality_framegraph_passes_ok=1`, `renderer_quality_framegraph_execution_budget_ok=1`, `renderer_quality_scene_gpu_ready=1`, and `renderer_quality_postprocess_ready=1`. |
| Installed Vulkan package smoke | PASS | `tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package -RequireVulkanShaders -SmokeArgs ... --require-vulkan-renderer --require-scene-gpu-bindings --require-postprocess --require-renderer-quality-gates` passed and installed validation reported the same generated 3D renderer quality fields on the Vulkan path with `renderer=vulkan`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | `production-readiness-audit-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | `format-check: ok`. |
| `git diff --check` | PASS | No whitespace errors; Git reported only CRLF conversion warnings for touched and pre-existing dirty files. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | `validate: ok`; CTest reported `100% tests passed, 0 tests failed out of 29`. Metal/iOS diagnostics remain host-gated on this Windows machine. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | `tools/build.ps1` completed the `dev` preset build with exit 0. |
