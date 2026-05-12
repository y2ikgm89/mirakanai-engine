# Generated 3D Playable Package Smoke v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a narrow generated `DesktopRuntime3DPackage` package smoke gate that proves the selected generated 3D runtime slice is playable through existing package, animation, scene GPU, postprocess, and renderer quality evidence.

**Architecture:** Reuse the committed/generated 3D package executable and installed package validator. The new `--require-playable-3d-slice` flag will aggregate existing first-party counters into `playable_3d_*` status fields; it will not add renderer/RHI APIs, pixel-readback claims, source importer execution, Metal parity, broad package streaming, or broad generated 3D production readiness.

**Tech Stack:** C++23, `SdlDesktopGameHost`, `SdlDesktopPresentationQualityGateReport`, generated `DesktopRuntime3DPackage`, PowerShell installed package validation, CMake registered desktop runtime game metadata.

---

## Context

- Master plan: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`.
- Previous generated 3D package slices added committed/generated package files, transform/morph/quaternion package smokes, D3D12/Vulkan compute morph package smokes, selected safe-point package streaming, and selected scene GPU + postprocess + framegraph=2 renderer quality.
- `engine/agent/manifest.json.aiOperableProductionLoop.unsupportedProductionGaps[3d-playable-vertical-slice]` is still `implemented-generated-desktop-3d-package-proof`.
- The next host-feasible advancement is a single package-visible playable-slice gate over the already validated generated 3D runtime pieces.

## Constraints

- Keep generated gameplay on public `mirakana::` APIs and host-owned SDL3 presentation contracts.
- Do not add new renderer/RHI APIs, public native handles, backend-native handles, pixel-readback requirements, or broad renderer quality claims.
- Do not claim runtime source asset parsing, broad dependency cooking, broad/background package streaming, production material/shader graph and live shader generation, Metal readiness, broad backend parity, editor productization, or broad generated 3D production readiness.
- Treat compute morph fields as selected package evidence when existing compute flags are present, not as a universal requirement for all generated 3D games.
- Update generated template output and committed sample together.
- Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` before completion reporting.

## Done When

- `sample_generated_desktop_runtime_3d_package --smoke ... --require-playable-3d-slice` reports:
  - `playable_3d_status=ready`
  - `playable_3d_ready=1`
  - `playable_3d_diagnostics=0`
  - `playable_3d_expected_frames=2`
  - `playable_3d_frames_ok=1`
  - `playable_3d_game_frames_ok=1`
  - `playable_3d_scene_mesh_plan_ready=1`
  - `playable_3d_camera_controller_ready=1`
  - `playable_3d_animation_ready=1`
  - `playable_3d_morph_ready=1`
  - `playable_3d_quaternion_ready=1`
  - `playable_3d_package_streaming_ready=1`
  - `playable_3d_scene_gpu_ready=1`
  - `playable_3d_postprocess_ready=1`
  - `playable_3d_renderer_quality_ready=1`
- `tools/validate-installed-desktop-runtime.ps1` validates those fields whenever smoke args include `--require-playable-3d-slice`, and validates selected compute evidence only when the corresponding compute flags are also present.
- `tools/new-game.ps1 -Template DesktopRuntime3DPackage` emits the same option, counters, manifest recipe args, and docs text.
- `games/sample_generated_desktop_runtime_3d_package/game.agent.json`, `games/CMakeLists.txt`, docs, static checks, and skills distinguish this generated 3D playable package smoke from broad generated 3D readiness.
- Focused build/smoke validation, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass or record concrete host/tool blockers.

## Tasks

### Task 1: RED

- [x] Run the committed generated 3D package smoke with `--require-playable-3d-slice` before implementation.
- [x] Record the expected failure: the argument is unknown or required `playable_3d_*` fields are absent.

### Task 2: Implement Committed Sample

- [x] Add `DesktopRuntimeOptions::require_playable_3d_slice` and parse `--require-playable-3d-slice`.
- [x] Add a generated-3D-specific playable aggregate report built from existing frame, scene mesh plan, camera, animation, morph, quaternion, package streaming, scene GPU, postprocess, renderer quality, and selected compute evidence.
- [x] Emit `playable_3d_*` fields on the existing status line and fail smoke when the required generated-3D playable gate is not ready.

### Task 3: Validation Script and Metadata

- [x] Update `tools/validate-installed-desktop-runtime.ps1` so installed validation enforces `playable_3d_*` fields when `--require-playable-3d-slice` is present.
- [x] Add the requirement to `games/CMakeLists.txt` installed package smoke args for `sample_generated_desktop_runtime_3d_package`.
- [x] Update `games/sample_generated_desktop_runtime_3d_package/game.agent.json` validation recipes and manifest text.

### Task 4: Generated Template

- [x] Update `tools/new-game.ps1` so generated `DesktopRuntime3DPackage` games contain the same option, counters, recipe args, manifest fields, and README text.

### Task 5: Docs and Static Checks

- [x] Update `docs/current-capabilities.md`, `docs/roadmap.md`, this registry, the master plan, `engine/agent/manifest.json`, and relevant skills.
- [x] Update `tools/check-ai-integration.ps1` to require the generated 3D playable package markers for committed and generated 3D package scaffolds.

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
| RED generated 3D package smoke with `--require-playable-3d-slice` | FAIL as expected | `out/build/desktop-runtime/.../sample_generated_desktop_runtime_3d_package.exe --smoke ... --require-playable-3d-slice` exited 1 with `unknown argument: --require-playable-3d-slice`. |
| Generated `DesktopRuntime3DPackage` scaffold | PASS | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` generated `desktop_3d_package_game` from `DesktopRuntime3DPackage`; `ai-integration-check: ok`. |
| Focused build/test | PASS | `cmake --build --preset desktop-runtime --target sample_generated_desktop_runtime_3d_package`; `ctest --preset desktop-runtime --output-on-failure -R sample_generated_desktop_runtime_3d_package` passed 2/2 tests. |
| Installed D3D12 package smoke | PASS | `tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package` completed with `playable_3d_status=ready`, `playable_3d_ready=1`, selected D3D12 compute morph skin and async telemetry evidence, and `installed-desktop-runtime-validation: ok`. |
| Installed Vulkan package smoke | PASS | `tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package -RequireVulkanShaders -SmokeArgs ... --require-vulkan-renderer --require-playable-3d-slice` completed with `playable_3d_status=ready`, `playable_3d_compute_morph_normal_tangent_ready=1`, and `desktop-runtime-package: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `ai-integration-check: ok`; generated `DesktopRuntime3DPackage` dry-run scaffold includes the playable gate markers. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | `production-readiness-audit-check: ok`; `3d-playable-vertical-slice` remains scoped as `implemented-generated-desktop-3d-package-proof`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Initial C++ formatting failure was corrected with `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`; rerun reported `format-check: ok`. |
| `git diff --check` | PASS | No whitespace errors; Git reported CRLF conversion warnings only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | `validate: ok`; CTest passed 29/29, with host-gated Metal/Apple diagnostics reported as expected non-fatal blockers. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | `tools/build.ps1` completed the `dev` preset build successfully. |
