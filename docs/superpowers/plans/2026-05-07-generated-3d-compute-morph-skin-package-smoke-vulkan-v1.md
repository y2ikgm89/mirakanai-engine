# Generated 3D Compute Morph Skin Package Smoke Vulkan v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Promote generated `DesktopRuntime3DPackage` Vulkan package smoke to cover compute-morphed POSITION output feeding GPU skinning, without exposing Vulkan/native handles, queue/fence internals, shader compiler execution, or async/performance claims to generated gameplay.

**Status:** Completed.

**Architecture:** Reuse the backend-neutral Runtime Scene RHI skin+compute bridge already proven through D3D12. Add host-owned Vulkan descriptor fields for skinned compute morph shader bytecode and explicit mesh bindings, validate the request through the SDL3 presentation boundary, dispatch compute output through `create_runtime_morph_mesh_compute_binding`, and render the compute-written POSITION stream through the skinned vertex layout/palette contract. Extend CMake shader metadata plus generated/committed `DesktopRuntime3DPackage` code so Vulkan SPIR-V artifacts are emitted, packaged, loaded, and required only for `--require-compute-morph-skin`.

**Tech Stack:** C++23, `mirakana_runtime_host_sdl3_presentation`, Runtime Scene RHI skinned compute morph bindings, Vulkan SPIR-V shader artifacts, CMake desktop runtime package metadata, `tools/new-game.ps1`, `mirakana_runtime_host_sdl3_tests`, AI integration checks.

---

## Goal

Close a narrow 3D production gap:

- Add Vulkan generated-package smoke evidence for compute morph POSITION output composed with GPU skinning.
- Keep POSITION-only and NORMAL/TANGENT Vulkan compute morph smokes working.
- Keep async telemetry and overlap/performance D3D12-only.
- Record that Vulkan async overlap/performance, Metal parity, directional-shadow morph rendering, scene-schema compute-morph authoring, public native handles, and broad renderer quality remain follow-up work.

## Context

- `mirakana_runtime_scene_rhi` already exposes `RuntimeSceneGpuBindingOptions::compute_morph_skinned_mesh_bindings` and retains those rows in `SceneSkinnedGpuBindingPalette`.
- `SceneGpuBindingInjectingRenderer` already reports compute-morph-skinned binding, dispatch, queue-wait, resolve, and draw counters.
- `SdlDesktopPresentationD3d12SceneRendererDesc` and the generated D3D12 package lane already pass `vs_compute_morph_skinned` / `cs_compute_morph_skinned_position` bytecode and explicit mesh-to-morph bindings.
- `SdlDesktopPresentationVulkanSceneRendererDesc`, generated `DesktopRuntime3DPackage`, and committed `sample_generated_desktop_runtime_3d_package` still reject `--require-compute-morph-skin` on the Vulkan lane and package no Vulkan skinned compute-morph SPIR-V artifacts.

## Constraints

- Do not expose `Vk*`, descriptor, queue, fence, shader module, pipeline, command buffer, command list, or RHI device handles through generated game APIs.
- Do not claim Vulkan async compute overlap/performance, Metal parity, directional-shadow morph rendering, scene-schema compute-morph authoring, public native handles, GPU timestamps, or broad renderer quality.
- Do not move shader compilation into runtime; CMake/package tooling owns shader artifact generation and validation.
- Keep D3D12 skin+compute and async telemetry package requirements working.
- Keep Vulkan POSITION-only and NORMAL/TANGENT compute morph package smokes working.
- Update Codex and Claude gameplay/rendering guidance together.

## Done When

- RED focused build proves `SdlDesktopPresentationVulkanSceneRendererDesc::compute_morph_skinned_shader` / `compute_morph_skinned_mesh_bindings` are missing.
- Vulkan SDL3 presentation can validate, dispatch, and render explicit skinned compute morph bindings through host-owned descriptor fields.
- CMake emits, validates, and installs Vulkan `*_scene_compute_morph_skinned.vs.spv` and `.cs.spv` artifacts when the scene helper receives skinned compute entries.
- `tools/new-game.ps1` generated `DesktopRuntime3DPackage` and committed `sample_generated_desktop_runtime_3d_package` load/select skinned Vulkan SPIR-V, allow `--require-compute-morph-skin` on the Vulkan lane, and continue rejecting `--require-compute-morph-async-telemetry` on Vulkan.
- Generated/committed manifests and validation recipes list the Vulkan skinned compute artifacts and include a Vulkan skin+compute package smoke recipe.
- Docs, master plan, registry, manifest, gameplay/rendering skills, and AI integration checks record the new boundary.
- Focused checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass before the slice commit.

## Tasks

### Task 1: RED Test

**Files:**
- Modify: `tests/unit/runtime_host_sdl3_tests.cpp`
- Modify: `tests/unit/runtime_host_sdl3_public_api_compile.cpp`

- [x] Add public API/test coverage for `SdlDesktopPresentationVulkanSceneRendererDesc::compute_morph_skinned_shader` and `compute_morph_skinned_mesh_bindings`.
- [x] Run a focused build and confirm it fails before implementation because the Vulkan descriptor fields are missing.

### Task 2: Host-Owned Vulkan Skin+Compute Rendering

**Files:**
- Modify: `engine/runtime_host/sdl3/include/mirakana/runtime_host/sdl3/sdl_desktop_presentation.hpp`
- Modify: `engine/runtime_host/sdl3/src/sdl_desktop_presentation.cpp`

- [x] Add the Vulkan descriptor fields and documentation.
- [x] Extend Vulkan scene-renderer request validation for skinned compute morph bindings.
- [x] Pass skinned compute morph mappings into Runtime Scene RHI binding creation.
- [x] Dispatch Vulkan skinned compute morph bindings and propagate dispatch/queue-wait counters into `SceneGpuBindingInjectingRenderer`.
- [x] Run focused `mirakana_runtime_host_sdl3_tests`.

### Task 3: Shader Artifact, Generator, And Committed Sample Alignment

**Files:**
- Modify: `games/CMakeLists.txt`
- Modify: `tools/new-game.ps1`
- Modify: `games/sample_generated_desktop_runtime_3d_package/main.cpp`
- Modify: `games/sample_generated_desktop_runtime_3d_package/game.agent.json`

- [x] Add Vulkan skinned compute morph shader artifact metadata and SPIR-V compile/validation commands.
- [x] Add generated/committed Vulkan skinned shader path constants and loader.
- [x] Select Vulkan skinned bytecode and compute-morph-skinned bindings when `--require-compute-morph-skin` is requested.
- [x] Remove the Vulkan skin rejection while retaining async telemetry rejection.
- [x] Add a Vulkan skin+compute package validation recipe.

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

- [x] Record Vulkan generated-package skin+compute morph package smoke as complete.
- [x] Keep Vulkan async overlap/performance, Metal parity, native handles, directional-shadow morph rendering, and broad renderer quality unsupported.
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
| RED focused build/test | Passed | `cmake --build --preset desktop-runtime --target mirakana_runtime_host_sdl3_tests mirakana_runtime_host_sdl3_public_api_compile` failed before implementation because `SdlDesktopPresentationVulkanSceneRendererDesc` had no `compute_morph_skinned_shader` or `compute_morph_skinned_mesh_bindings` members. |
| Focused `mirakana_runtime_host_sdl3_tests` | Passed | `cmake --build --preset desktop-runtime --target mirakana_runtime_host_sdl3_tests mirakana_runtime_host_sdl3_public_api_compile sample_generated_desktop_runtime_3d_package sample_generated_desktop_runtime_3d_package_vulkan_shaders sample_generated_desktop_runtime_3d_package_vulkan_shader_validation` passed, then `ctest --preset desktop-runtime --output-on-failure -R "mirakana_runtime_host_sdl3_tests|mirakana_runtime_host_sdl3_public_api_compile"` passed 2/2. |
| Vulkan shader/package smoke | Passed | `tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package -RequireVulkanShaders` passed with `--require-compute-morph-skin`, reporting `renderer=vulkan`, `scene_gpu_status=ready`, `scene_gpu_compute_morph_skinned_mesh_bindings=1`, `scene_gpu_compute_morph_skinned_dispatches=1`, `scene_gpu_compute_morph_skinned_queue_waits=1`, `scene_gpu_compute_morph_skinned_mesh_resolved=2`, `scene_gpu_compute_morph_skinned_draws=2`, and `renderer_gpu_skinning_draws=2`. The existing Vulkan NORMAL/TANGENT package smoke was rerun and still reported `scene_gpu_compute_morph_tangent_frame_output=1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | `tools/check-ai-integration.ps1` completed with `ai-integration-check: ok` after checking docs, manifest, generator, runtime-host public API, and Codex/Claude skills. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Passed | Reported the unsupported gap summary and completed with `production-readiness-audit-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed | Failed once on clang-format layout in `sdl_desktop_presentation.cpp`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` applied repository formatting, then `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed | Public API boundary check passed after adding the Vulkan descriptor fields behind the SDL3 presentation contract. |
| `git diff --check` | Passed | Whitespace check passed; Git reported line-ending normalization warnings only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Full validation completed with `validate: ok`; Windows host-gated Metal/Apple diagnostics were reported as diagnostic-only/host-gated. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Passed | Dev preset configure/build completed successfully. |
| Slice-closing commit | Passed | Stage only this slice's implementation, docs, manifest, sample, generator, test, and guidance files; leave unrelated pre-existing dirty files unstaged. |
