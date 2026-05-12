# Generated 3D Postprocess Depth Package Smoke v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a narrow generated `DesktopRuntime3DPackage` package smoke gate that proves the selected generated 3D postprocess path can request and validate a renderer-owned scene-depth input.

**Architecture:** Reuse the existing SDL3 host-owned postprocess depth-input contract and renderer quality gate. The generated 3D package executable and template will accept `--require-postprocess-depth-input`, enable depth input on D3D12/Vulkan scene renderer descriptors, emit `postprocess_depth_input_ready`, and keep the ready claim scoped to the generated package smoke.

**Tech Stack:** C++23, `SdlDesktopGameHost`, `SdlDesktopPresentation*SceneRendererDesc`, HLSL/DXIL/SPIR-V package shader artifacts, PowerShell installed package validation, CMake registered desktop runtime game metadata.

---

## Context

- Master plan: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`.
- Previous generated 3D package slices added committed/generated package files, transform/morph/quaternion package smokes, D3D12/Vulkan compute morph package smokes, selected safe-point package streaming, renderer quality over scene GPU + postprocess + framegraph=2, and `playable_3d_*` aggregate counters.
- `tools/validate-installed-desktop-runtime.ps1` already recognizes `--require-postprocess-depth-input` and expects `postprocess_depth_input_ready=1` plus `renderer_quality_postprocess_depth_input_ready=1`, but `sample_generated_desktop_runtime_3d_package` does not currently accept the flag or emit the direct depth-input field.
- This slice is the smallest host-feasible generated 3D renderer-quality advancement before directional shadow work.

## Constraints

- Keep the depth-input path host-owned behind `SdlDesktopPresentation*SceneRendererDesc`; do not expose native GPU/RHI handles to gameplay.
- Do not add directional shadows, shadow filtering, GPU timestamps, backend-native stats, pixel readback, frame graph scheduling claims, Metal parity, broad renderer quality, or broad generated 3D production readiness.
- Keep `framegraph_passes=2` for the generated package depth-input smoke; this is a depth-aware postprocess pass, not the directional-shadow 3-pass renderer.
- Update generated template output and committed sample together.
- Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` before completion reporting and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 && pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` before the slice-closing commit.

## Done When

- `sample_generated_desktop_runtime_3d_package --smoke ... --require-postprocess-depth-input --require-renderer-quality-gates` reports:
  - `postprocess_status=ready`
  - `postprocess_depth_input_ready=1`
  - `framegraph_passes=2`
  - `renderer_quality_status=ready`
  - `renderer_quality_ready=1`
  - `renderer_quality_diagnostics=0`
  - `renderer_quality_expected_framegraph_passes=2`
  - `renderer_quality_postprocess_depth_input_ready=1`
- `tools/validate-installed-desktop-runtime.ps1` accepts the generated 3D package smoke args with `--require-postprocess-depth-input`.
- `tools/new-game.ps1 -Template DesktopRuntime3DPackage` emits the same option, descriptor wiring, status field, manifest recipe args, and README text.
- `games/sample_generated_desktop_runtime_3d_package/game.agent.json`, `games/CMakeLists.txt`, docs, static checks, and skills distinguish this generated 3D postprocess-depth smoke from directional shadows, broad renderer quality, Metal parity, and broad generated 3D readiness.
- Focused build/smoke validation, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass or record concrete host/tool blockers.

## Tasks

### Task 1: RED

- [x] Run the committed generated 3D package smoke with `--require-postprocess-depth-input` before implementation.
- [x] Record the expected failure: the argument is unknown or required depth-input fields are absent.

### Task 2: Implement Committed Sample

- [x] Add `DesktopRuntimeOptions::require_postprocess_depth_input` and parse `--require-postprocess-depth-input`, implying `--require-postprocess`.
- [x] Pass `enable_postprocess_depth_input` to both D3D12 and Vulkan generated 3D scene renderer descriptors when requested.
- [x] Extend renderer quality gate desc requirements and smoke failure checks for `postprocess_depth_input_ready`.
- [x] Emit direct `postprocess_depth_input_ready` on the generated 3D status line.
- [x] Update `runtime_postprocess.hlsl` to bind and sample `scene_depth_texture` at `t2/s3` so shader artifacts exercise the depth-input descriptor contract.

### Task 3: Validation Script and Metadata

- [x] Add `--require-postprocess-depth-input` to `games/CMakeLists.txt` installed package smoke args for `sample_generated_desktop_runtime_3d_package`.
- [x] Update `games/sample_generated_desktop_runtime_3d_package/game.agent.json` validation recipes and manifest text.
- [x] Confirm `tools/validate-installed-desktop-runtime.ps1` enforces `postprocess_depth_input_ready=1` and renderer quality depth-input fields for the generated package smoke args.

### Task 4: Generated Template

- [x] Update `tools/new-game.ps1` so generated `DesktopRuntime3DPackage` games contain the same option, descriptor wiring, status output, README text, CMake package smoke args, and manifest validation recipe args.

### Task 5: Docs and Static Checks

- [x] Update `docs/current-capabilities.md`, `docs/roadmap.md`, this registry, the master plan, `engine/agent/manifest.json`, and relevant skills.
- [x] Update `tools/check-ai-integration.ps1` to require generated 3D postprocess-depth package smoke markers for committed and generated 3D package scaffolds.

### Task 6: Validate and Commit

- [x] Run focused build/test and package smoke.
- [x] Run a strict Vulkan package smoke with `--require-postprocess-depth-input` on this Vulkan-ready Windows host.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` before the slice-closing commit.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED generated 3D package smoke with `--require-postprocess-depth-input` | FAIL as expected | `out/build/desktop-runtime/.../sample_generated_desktop_runtime_3d_package.exe --smoke ... --require-postprocess-depth-input --require-renderer-quality-gates` exited 1 with `unknown argument: --require-postprocess-depth-input`. |
| `cmake --build --preset desktop-runtime --target sample_generated_desktop_runtime_3d_package` | PASS | Focused target build passed after adding the option, descriptor wiring, and shader binding. |
| Direct D3D12 generated 3D package smoke with `--require-postprocess-depth-input --require-renderer-quality-gates` | PASS | Reported `postprocess_status=ready`, `postprocess_depth_input_ready=1`, `framegraph_passes=2`, `renderer_quality_status=ready`, `renderer_quality_ready=1`, and `renderer_quality_postprocess_depth_input_ready=1`. |
| `tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package` | PASS | Installed D3D12 package smoke passed with `postprocess_depth_input_ready=1`, `renderer_quality_postprocess_depth_input_ready=1`, and `playable_3d_status=ready`; script ended with `desktop-runtime-package: ok`. |
| Strict Vulkan package smoke with `-RequireVulkanShaders` and `--require-postprocess-depth-input` | PASS | Installed Vulkan package smoke passed with `renderer=vulkan`, `postprocess_depth_input_ready=1`, `renderer_quality_postprocess_depth_input_ready=1`, and `playable_3d_status=ready`; script ended with `desktop-runtime-package: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | First run exposed a stale generated-skill phrase expectation; after aligning the skill text, rerun passed with `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | `production-readiness-audit-check: ok`; unsupported gap count/status summary stayed explicit. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | `format-check: ok`. |
| `git diff --check` | PASS | No whitespace errors; Git reported existing LF-to-CRLF working-copy warnings. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full validation passed after JSON contract, agent integration, production readiness audit, toolchain checks, tidy smoke, build, and CTest; Metal/Apple diagnostics remain host-gated as expected on Windows. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Dev preset build passed before the slice-closing commit. |
