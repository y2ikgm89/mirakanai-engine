# Desktop Runtime Productization Implementation Plan (2026-05-01)

> **For agentic workers:** This completed plan is implementation evidence. Do not append new production work here; create a new dated focused plan for the next slice.

**Goal:** Productize the first practical Windows desktop vertical slice so a game can be created, launched from the source tree, packaged, installed, and validated through one documented desktop runtime path.

**Architecture:** Keep the productized lane centered on `sample_desktop_runtime_game`, `mirakana::SdlDesktopGameHost`, manifest-derived `runtimePackageFiles`, and metadata-driven package validation. D3D12 is the primary verified Windows lane. Vulkan is validated on this ready Windows host through the strict selected package command, but remains host/runtime/toolchain-gated elsewhere. Apple/iOS/Metal remains outside this slice. Native OS, SDL3, RHI, D3D12, Vulkan, and Dear ImGui handles stay behind platform/runtime-host/renderer backends and out of public game APIs.

**Tech Stack:** C++23, CMake presets, CTest, PowerShell 7 validation scripts under `tools/`, SDL3 desktop runtime host, D3D12 DXIL shader artifacts, host-gated Vulkan SPIR-V shader artifacts, cooked `GameEngine.Scene.v1` packages, `engine/agent/manifest.json`, `docs/roadmap.md`, and desktop runtime game manifests.

---

## Goal

Make the desktop runtime path honest and repeatable as the first practical vertical slice after `core-first-mvp`: a representative game can run in the source tree, load packaged config and cooked scene content, bind scene GPU resources on the primary Windows D3D12 package lane, execute depth-aware postprocess and fixed 3x3 PCF directional-shadow smoke checks, install as a desktop runtime package, and report concrete validation evidence.

## Context

- `core-first-mvp` is closed by `docs/superpowers/plans/2026-05-01-core-first-mvp-closure.md` as an MVP scope, not as a complete commercial engine.
- `sample_desktop_runtime_game` declares `desktop-game-runtime` and `desktop-runtime-release`, manifest-derived package files, D3D12 package smoke args, and a host-gated Vulkan package recipe.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` is the source-tree desktop runtime lane and validates registered runtime game targets plus runtime host, SDL3 platform/audio, presentation, generated scaffold, and sample smokes.
- `tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game` is the representative non-shell package proof; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1` remains the default sample-shell package lane.
- Vulkan package strictness depends on a Windows host with Vulkan runtime, DXC SPIR-V CodeGen, and `spirv-val`. Those gates were ready on this host and the strict selected package command passed.
- Apple/iOS/Metal productization remains host-gated and is not part of this slice.

## Constraints

- Do not reopen or append unrelated tasks to the closed `core-first-mvp` plan.
- Do not introduce third-party dependencies for this slice.
- Do not make `engine/core` depend on platform, renderer, asset format, editor, SDL3, Dear ImGui, OS, or GPU APIs.
- Do not expose SDL3, Win32, D3D12, Vulkan, Metal, RHI device, or Dear ImGui handles through public game APIs.
- Keep `sample_desktop_runtime_game` a representative smoke/productization sample, not a claim of production renderer, production asset pipeline, material graph, editor workflow, or commercial shipping completeness.
- Treat Vulkan and Metal as host-gated unless the exact local or CI command proves readiness.

## Done When

- [x] `docs/superpowers/plans/README.md` marks this plan as the active slice while work is underway and completed after verification.
- [x] Baseline `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` was run before implementation and passed.
- [x] Baseline `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` was run before implementation and passed.
- [x] Baseline `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` was run and validated source-tree registered runtime games.
- [x] Baseline `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1` was run; sandboxed vcpkg extraction failed, then the same command passed outside the sandbox.
- [x] `sample_desktop_runtime_game` was validated as the representative non-shell package lane with `tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game`.
- [x] Installed desktop runtime validation requires the selected game status line before accepting presentation, scene GPU, postprocess, or directional-shadow report fields.
- [x] `docs/roadmap.md` separates the completed desktop runtime productization scope from next-phase items such as Metal presentation, broader material/shader authoring, production renderer features, editor UX expansion, and broader asset pipeline conventions.
- [x] `engine/agent/manifest.json` honestly represents desktop runtime/package/sample readiness, the D3D12 primary verified lane, Vulkan host gating, and Apple/iOS/Metal next-phase gating.
- [x] Public renderer header changes were followed by `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Final `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` passed.
- [x] This plan records validation evidence and the plan registry marks the slice completed after verification.

## Implementation Tasks

### Task 1: Baseline Desktop Runtime Readiness

- [x] Read `games/CMakeLists.txt`, `games/sample_desktop_runtime_game/game.agent.json`, `games/sample_desktop_runtime_game/README.md`, `tools/validate-desktop-game-runtime.ps1`, and `tools/package-desktop-runtime.ps1`.
- [x] Ran `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS.
- [x] Ran `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`: PASS.
- [x] Ran `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`: PASS.
- [x] Ran `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1`: sandbox vcpkg extraction failed, then elevated rerun PASS.
- [x] Ran `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game`: sandbox vcpkg extraction failed, then elevated rerun PASS.
- [x] Classified failures: the package failures were sandbox-only vcpkg 7zip extraction (`CreateFileW stdin failed with 5 (Access is denied.)`), not source defects.

### Task 2: Source-Tree And Package Contract Corrections

- [x] Added a negative installed-runtime fixture using `mirakana_process_probe.exe` as a fake selected game; before the fix, `tools/validate-installed-desktop-runtime.ps1` accepted a successful process with no `<target> status=` line.
- [x] Updated `tools/validate-installed-desktop-runtime.ps1` to require the selected game status line before accepting presentation, scene GPU, postprocess, or directional-shadow fields.
- [x] Updated `tools/check-ai-integration.ps1` to statically assert the required status-line diagnostic exists.
- [x] Confirmed the fake installed-runtime fixture now fails with `Installed desktop runtime smoke did not emit the required 'fake_game status=' report line.`
- [x] Added renderer lifecycle tests proving `RhiFrameRenderer` and `RhiPostprocessFrameRenderer` release unpresented acquired swapchain frames on destruction.
- [x] Confirmed focused renderer RED: `mirakana_renderer_tests` failed on both new destruction tests before the renderer fix.
- [x] Implemented destructor cleanup helpers for `RhiFrameRenderer` and `RhiPostprocessFrameRenderer`, preserving the rule that presented frames are not manually released.
- [x] Confirmed focused renderer GREEN: `mirakana_renderer_tests` passed.
- [x] Ran `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: PASS.

### Task 3: Roadmap And Manifest Synchronization

- [x] Updated `docs/roadmap.md` to mark this slice complete and keep next-phase items separate.
- [x] Updated `docs/workflows.md`, `docs/testing.md`, `docs/ai-game-development.md`, sample README guidance, and `engine/agent/manifest.json` to describe selected-game status-line enforcement.
- [x] Kept D3D12 described as the primary verified Windows package lane.
- [x] Kept Vulkan described as host/runtime/toolchain-gated, with strict selected package validation passed on this host.
- [x] Kept Apple/iOS/Metal outside this slice as macOS/Xcode/Metal host-gated next-phase work.
- [x] Kept next-phase items concrete: Metal presentation, broader material/shader graph/live generation, production postprocess/shadow quality, broader cooked asset conventions, editor UX, concrete runtime UI adapters, telemetry/crash reporting, allocator diagnostics, and GPU marker adapters.
- [x] Ran `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS.

### Task 4: Final Validation And Closure

- [x] Ran `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`: PASS, 12/12 tests.
- [x] Ran `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1`: sandbox vcpkg extraction failed, elevated rerun PASS with 7/7 tests, installed SDK validation, installed desktop runtime validation, CPack ZIP, and selected shell status line.
- [x] Ran `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game`: sandbox vcpkg extraction failed, elevated rerun PASS with selected D3D12 installed status fields.
- [x] Ran strict Vulkan selected package validation through `tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game -RequireVulkanShaders -SmokeArgs ...`: PASS on this ready Windows host with selected Vulkan installed status fields.
- [x] Ran `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS, 22/22 CTest tests.
- [x] Ran `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`: PASS.
- [x] Ran `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: PASS.
- [x] Recorded command outcomes in this plan.
- [x] Marked this plan completed in the registry.

## Validation Evidence

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`: PASS before planning.
- Baseline `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS, 22/22 CTest tests. Diagnostic-only gates remained explicit: Metal tools missing, Apple packaging requires macOS/Xcode, Android signing/device smoke not configured/connected, strict tidy compile database unavailable at tidy-check start.
- Baseline `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`: PASS.
- Baseline `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`: PASS after sandbox-free vcpkg state, 12/12 tests.
- Baseline `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1`: sandbox run failed at vcpkg 7zip extraction with `CreateFileW stdin failed with 5 (Access is denied.)`; elevated rerun PASS with 7/7 tests, installed SDK validation, installed desktop runtime validation, and CPack ZIP generation.
- Baseline selected package `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game`: sandbox run failed with the same vcpkg 7zip extraction blocker; elevated rerun PASS with installed `sample_desktop_runtime_game status=completed`, `renderer=d3d12`, `scene_gpu_status=ready`, `postprocess_depth_input_ready=1`, `directional_shadow_status=ready`, `directional_shadow_ready=1`, `directional_shadow_filter_mode=fixed_pcf_3x3`, `directional_shadow_filter_taps=9`, `directional_shadow_filter_radius_texels=1`, and `framegraph_passes=3`.
- Installed status-line negative fixture: before the fix, a fake executable with no status output passed installed validation; after the fix, it fails with `Installed desktop runtime smoke did not emit the required 'fake_game status=' report line.`
- Focused renderer lifecycle RED: `mirakana_renderer_tests` failed on `rhi postprocess frame renderer releases acquired swapchain frame on destruction` and `rhi frame renderer releases acquired swapchain frame on destruction`.
- Focused renderer lifecycle GREEN: `mirakana_renderer_tests` PASS after destructor cleanup helpers were added.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: PASS.
- Final `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`: PASS, 12/12 tests.
- Final `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1`: sandbox vcpkg extraction failed, elevated rerun PASS with selected shell status line, installed SDK validation, installed desktop runtime validation, and CPack ZIP generation.
- Final selected D3D12 package `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game`: sandbox vcpkg extraction failed, elevated rerun PASS with selected D3D12 status line and package artifacts.
- Strict selected Vulkan package direct script call with `-RequireVulkanShaders` and explicit Vulkan smoke args: PASS on this Windows host with selected Vulkan status line, SPIR-V shader artifacts, `scene_gpu_status=ready`, `postprocess_depth_input_ready=1`, `directional_shadow_ready=1`, fixed 3x3 PCF fields, and `framegraph_passes=3`.
- Final `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS, 22/22 CTest tests. Diagnostic-only host gates remained explicit: Metal `metal`/`metallib` missing, Apple packaging blocked by missing macOS/Xcode tools, Android release signing not configured, Android device smoke not connected, and strict tidy analysis blocked by missing compile database at tidy-check start.
- Final `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`: PASS.
