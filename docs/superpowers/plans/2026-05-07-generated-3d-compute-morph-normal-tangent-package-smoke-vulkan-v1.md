# Generated 3D Compute Morph NORMAL/TANGENT Package Smoke Vulkan v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Promote generated `DesktopRuntime3DPackage` Vulkan compute morph package smoke from POSITION-only to explicit NORMAL/TANGENT output rendering evidence, without exposing Vulkan handles, queue/fence internals, shader compiler execution, or async/performance claims to generated gameplay.

**Architecture:** Reuse the existing host-owned SDL3 presentation compute morph path. Add a Vulkan scene-renderer descriptor switch matching the D3D12 tangent-frame request, pass it into `build_scene_compute_morph_bindings` so Runtime RHI creates output NORMAL/TANGENT buffers, and select tangent-frame vertex layout/attributes for the render pipeline. Extend the root CMake shader helper plus the `DesktopRuntime3DPackage` generator and committed sample metadata to emit, package, validate, load, and require the Vulkan tangent-frame SPIR-V pair only when `--require-compute-morph-normal-tangent` is requested.

**Tech Stack:** C++23, `mirakana_runtime_host_sdl3_presentation`, Runtime RHI compute morph bindings, Vulkan SPIR-V shader artifacts, CMake desktop runtime package metadata, `tools/new-game.ps1`, `mirakana_runtime_host_sdl3_tests`, AI integration checks.

---

## Goal

Close a narrow 3D production gap:

- Add Vulkan NORMAL/TANGENT compute morph output rendering to the host-owned generated package smoke path.
- Keep POSITION-only Vulkan compute morph package smoke compatible.
- Keep D3D12 skin+compute and async telemetry D3D12-only.
- Record that Vulkan skin+compute package smoke, async overlap/performance, Metal parity, directional-shadow morph rendering, scene-schema compute-morph authoring, public native handles, and broad renderer quality remain follow-up work.

## Context

- `RuntimeMorphMeshComputeBindingOptions::output_normal_usage` and `output_tangent_usage` already work on Vulkan in runtime/RHI proof tests.
- `SdlDesktopPresentationD3d12SceneRendererDesc` already exposes `enable_compute_morph_tangent_frame_output` and uses tangent-frame vertex bindings for D3D12 package smoke.
- `SdlDesktopPresentationVulkanSceneRendererDesc`, generated `DesktopRuntime3DPackage`, and committed `sample_generated_desktop_runtime_3d_package` still reject `--require-compute-morph-normal-tangent` on the Vulkan lane and package only POSITION compute morph SPIR-V artifacts.

## Constraints

- Do not expose `Vk*`, descriptor, queue, fence, shader module, pipeline, command list, or RHI device handles through generated game APIs.
- Do not claim Vulkan skin+compute package smoke, Vulkan async compute overlap/performance, Metal parity, directional-shadow morph rendering, scene-schema compute-morph authoring, or broad renderer quality.
- Do not move shader compilation into runtime; CMake/package tooling owns shader artifact generation and validation.
- Keep D3D12 package requirements and POSITION-only Vulkan smoke behavior working.
- Update Codex and Claude gameplay/rendering guidance together.

## Done When

- RED focused build proves `SdlDesktopPresentationVulkanSceneRendererDesc::enable_compute_morph_tangent_frame_output` is missing.
- Vulkan SDL3 presentation can request tangent-frame compute morph output, dispatch Runtime RHI NORMAL/TANGENT output, and render with tangent-frame vertex buffers/attributes.
- CMake emits and validates Vulkan `*_scene_compute_morph_tangent_frame.vs.spv` and `.cs.spv` artifacts when the scene helper receives tangent-frame entries.
- `tools/new-game.ps1` generated `DesktopRuntime3DPackage` and committed `sample_generated_desktop_runtime_3d_package` load/select tangent-frame Vulkan SPIR-V, allow `--require-compute-morph-normal-tangent` on the Vulkan lane, and require `scene_gpu_compute_morph_tangent_frame_output`.
- Generated/committed manifests and validation recipes list the Vulkan tangent-frame artifacts and pass `--require-compute-morph-normal-tangent` for the Vulkan toolchain-gated smoke.
- Docs, master plan, registry, manifest, gameplay/rendering skills, and AI integration checks record the new boundary.
- Focused checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass before the slice commit.

## Tasks

### Task 1: RED Test

**Files:**
- Modify: `tests/unit/runtime_host_sdl3_tests.cpp`
- Modify: `tests/unit/runtime_host_sdl3_public_api_compile.cpp`

- [x] Add public API/test coverage for `SdlDesktopPresentationVulkanSceneRendererDesc::enable_compute_morph_tangent_frame_output`.
- [x] Run `cmake --build --preset desktop-runtime --target mirakana_runtime_host_sdl3_tests` and confirm it fails before implementation because the Vulkan descriptor field is missing.

### Task 2: Host-Owned Vulkan Tangent-Frame Compute Morph Rendering

**Files:**
- Modify: `engine/runtime_host/sdl3/include/mirakana/runtime_host/sdl3/sdl_desktop_presentation.hpp`
- Modify: `engine/runtime_host/sdl3/src/sdl_desktop_presentation.cpp`

- [x] Add the Vulkan descriptor switch and documentation.
- [x] Pass the switch into `build_scene_compute_morph_bindings`.
- [x] Select tangent-frame compute morph vertex buffers/attributes when requested.
- [x] Run focused `mirakana_runtime_host_sdl3_tests`.

### Task 3: Shader Artifact, Generator, And Committed Sample Alignment

**Files:**
- Modify: `games/CMakeLists.txt`
- Modify: `tools/new-game.ps1`
- Modify: `games/sample_generated_desktop_runtime_3d_package/main.cpp`
- Modify: `games/sample_generated_desktop_runtime_3d_package/game.agent.json`

- [x] Add Vulkan tangent-frame shader artifact metadata and SPIR-V compile/validation commands.
- [x] Add generated/committed Vulkan tangent-frame shader path constants and loader.
- [x] Select Vulkan tangent-frame bytecode when `--require-compute-morph-normal-tangent` is requested.
- [x] Remove the Vulkan POSITION-only rejection for NORMAL/TANGENT while retaining skin and async telemetry rejection.
- [x] Add `--require-compute-morph-normal-tangent` to Vulkan package validation recipes.

### Task 4: Docs, Manifest, Guidance, Static Checks

**Files:**
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`
- Modify: `.agents/skills/rendering-change/SKILL.md`
- Modify: `.claude/skills/gameengine-rendering/SKILL.md`

- [x] Record Vulkan generated-package NORMAL/TANGENT compute morph package smoke as complete.
- [x] Keep Vulkan skin+compute package smoke, async overlap/performance, Metal parity, native handles, and broad renderer quality unsupported.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.

### Task 5: Final Validation And Commit

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Create a slice-closing commit after validation/build passes, staging only relevant files.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED focused build/test | Passed | `cmake --build --preset desktop-runtime --target mirakana_runtime_host_sdl3_tests` failed before implementation after emitting the expected missing `SdlDesktopPresentationVulkanSceneRendererDesc::enable_compute_morph_tangent_frame_output` compile error. |
| Focused `mirakana_runtime_host_sdl3_tests` | Passed | `cmake --build --preset desktop-runtime --target mirakana_runtime_host_sdl3_tests mirakana_runtime_host_sdl3_public_api_compile sample_generated_desktop_runtime_3d_package`; `ctest --preset desktop-runtime --output-on-failure -R "mirakana_runtime_host_sdl3_tests|mirakana_runtime_host_sdl3_public_api_compile"` passed. |
| Vulkan shader/package smoke | Passed | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1` reported D3D12 DXIL and Vulkan SPIR-V ready; `cmake --build --preset desktop-runtime --target sample_generated_desktop_runtime_3d_package_vulkan_shaders sample_generated_desktop_runtime_3d_package_vulkan_shader_validation` passed; selected installed Vulkan package smoke passed with `renderer=vulkan`, `scene_gpu_status=ready`, and `scene_gpu_compute_morph_tangent_frame_output=1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Passed | `production-readiness-audit-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed | `format-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed | `public-api-boundary-check: ok`. |
| `git diff --check` | Passed | No whitespace errors; Git reported only existing LF/CRLF conversion warnings. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | `validate: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Passed | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 && pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` exited 0. |
| Slice-closing commit | Passed | This slice-closing commit stages only the Vulkan NORMAL/TANGENT package smoke files and leaves unrelated pre-existing dirty files unstaged. |
