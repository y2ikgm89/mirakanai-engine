# Package-Visible Postprocess Depth Effect v0 Implementation Plan (2026-04-30)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Promote the existing postprocess depth-input foundation into a package-visible SDL3 desktop runtime proof that can request, report, and smoke-validate a depth-aware postprocess path.

**Architecture:** Keep the sampled scene depth texture, sampler, descriptor set, graphics pipeline, swapchain frame, and native backend objects owned by `mirakana_renderer`, `mirakana_runtime_host_sdl3_presentation`, and private RHI backends. Expose only first-party request flags, diagnostics, readiness booleans, and `IRenderer::stats()` counters through runtime-host contracts. The sample shader may sample scene depth at stable bindings `t2/s3`, but gameplay code must not receive native handles or `IRhiDevice`.

**Tech Stack:** C++23, SDL3 desktop runtime host adapter, `mirakana_renderer::RhiPostprocessFrameRenderer`, D3D12 DXIL package lane, Vulkan SPIR-V package lane when toolchain/runtime ready, first-party HLSL samples, PowerShell validation scripts.

---

## Context

- `RhiPostprocessFrameRenderer` already supports opt-in renderer-owned `depth24_stencil8` sampled scene depth with stable scene color bindings `0/1` and scene depth bindings `2/3`.
- `sample_desktop_runtime_game` already packages scene and postprocess shader artifacts, but its current postprocess shader is color-only and runtime smoke only checks `postprocess_status=ready` plus `framegraph_passes=2`.
- Unity documents camera depth textures as inputs for post-processing effects, and Unreal documents post-process materials plus scene-depth lookup APIs. This slice implements only the minimal equivalent package-visible proof, not a full effect stack.
- Read-only subagents selected this as the highest-impact host-feasible follow-up and flagged the main blockers: SDL3 scene pipelines are depthless, runtime-host descs have no depth-input opt-in, package shaders do not read `t2/s3`, and installed smokes cannot distinguish depth-aware postprocess.

## Constraints

- Keep public API under `mirakana::`; no SDL3, OS, D3D12, Vulkan, Metal, Dear ImGui, editor, `IRhiDevice`, swapchain frame, descriptor handle, texture view, image view, or native backend object in gameplay/public game APIs.
- Do not add third-party dependencies.
- Do not claim SSAO, depth of field, fog, temporal history, bloom, postprocess material graphs, package-visible shadows, Metal postprocess depth, GPU markers, or full render-graph scheduling.
- Public runtime-host header changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- Shader/RHI/renderer/package changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`.
- Desktop runtime/package changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` and selected `tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game`.
- End the slice with focused validation, cpp-reviewer, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

## Files

- Modify: `engine/runtime_host/sdl3/include/mirakana/runtime_host/sdl3/sdl_desktop_presentation.hpp` for first-party depth-input request/report fields.
- Modify: `engine/runtime_host/sdl3/src/sdl_desktop_presentation.cpp` to create depth-enabled scene pipelines and pass `enable_depth_input` to `RhiPostprocessFrameRenderer`.
- Modify: `tests/unit/runtime_host_sdl3_tests.cpp` for RED/green request/report/fallback behavior.
- Modify: `tests/unit/runtime_host_sdl3_public_api_compile.cpp` to keep the first-party public contract compile-proven.
- Modify: `games/sample_desktop_runtime_game/main.cpp` and `games/sample_desktop_runtime_game/shaders/runtime_postprocess.hlsl` for `--require-postprocess-depth-input`, status output, and `t2/s3` depth sampling.
- Modify: `games/sample_desktop_runtime_game/README.md`, `games/sample_desktop_runtime_game/game.agent.json`, `games/CMakeLists.txt`, and `tools/validate-installed-desktop-runtime.ps1` for package smoke evidence.
- Modify generated material/shader package sample and template only if the shared guidance or validation demands it: `games/sample-generated-desktop-runtime-material-shader-package/*` and `tools/new-game.ps1`.
- Modify docs/AI sync: `docs/rhi.md`, `docs/roadmap.md`, `docs/testing.md`, `docs/specs/2026-04-27-engine-essential-gap-analysis.md`, `engine/agent/manifest.json`, `tools/check-ai-integration.ps1`, `.agents/skills/rendering-change/SKILL.md`, `.claude/skills/gameengine-rendering/SKILL.md`, `.codex/agents/rendering-auditor.toml`, `.claude/agents/rendering-auditor.md`.

## Tasks

### Task 1: RED Runtime-Host Depth Request Tests

- [x] Add a `runtime_host_sdl3_tests` case where a D3D12 scene renderer requests `enable_postprocess_depth_input = true` with a dummy SDL surface and expects `postprocess_status=unavailable`, `postprocess_depth_input_requested=true`, `postprocess_depth_input_ready=false`, and a postprocess diagnostic mentioning depth input.
- [x] Add the equivalent Vulkan null-fallback test.
- [x] Add public API compile coverage for the new request/report fields without introducing native handles.
- [x] Run focused CTest and verify the new tests fail because the fields do not exist yet.

### Task 2: Runtime-Host Depth Pipeline Wiring

- [x] Add `enable_postprocess_depth_input` to D3D12 and Vulkan scene renderer descs.
- [x] Add `postprocess_depth_input_requested` and `postprocess_depth_input_ready` to `SdlDesktopPresentationReport`, plus a `postprocess_depth_input_ready()` accessor.
- [x] In D3D12 and Vulkan scene renderer creation, use `depth24_stencil8` plus `DepthStencilStateDesc{true, true, less_equal}` only when depth input is requested.
- [x] Construct `RhiPostprocessFrameRendererDesc` with `.enable_depth_input = true` and `.depth_format = depth24_stencil8` when requested.
- [x] Ensure missing shader, invalid request, native-surface unavailable, and creation-failure paths preserve honest diagnostics and never report depth ready on `NullRenderer`.
- [x] Re-run focused runtime-host tests until they pass.

### Task 3: Sample Game Depth-Aware Package Smoke

- [x] Update `sample_desktop_runtime_game` CLI usage and parsing for `--require-postprocess-depth-input`; make it imply `--require-postprocess`.
- [x] Pass `enable_postprocess_depth_input=true` into D3D12/Vulkan scene renderer descs for the sample when postprocess is enabled.
- [x] Print `postprocess_depth_input_ready=0|1` on the status and presentation report lines.
- [x] In smoke mode, fail when `--require-postprocess-depth-input` is set and the report is not depth-ready.
- [x] Update `runtime_postprocess.hlsl` to sample scene depth at `t2/s3` and apply a small deterministic depth-derived grade while preserving color-only bindings.
- [x] Run the sample shader target build and source-tree desktop runtime smoke.

### Task 4: Package Metadata and Installed Validation

- [x] Update `games/CMakeLists.txt` package smoke args for `sample_desktop_runtime_game` to require postprocess depth input on the D3D12 selected package lane.
- [x] Update `tools/validate-installed-desktop-runtime.ps1` so a ready scene GPU postprocess smoke must include `postprocess_depth_input_ready=1` when the smoke args request it.
- [x] Update `games/sample_desktop_runtime_game/game.agent.json` validation recipes, backend readiness, and runtime contract wording.
- [x] Run selected D3D12 package validation for `sample_desktop_runtime_game`.
- [x] Run Vulkan selected package validation with `-RequireVulkanShaders` if the host remains Vulkan toolchain/runtime ready; otherwise record exact host/toolchain gate and keep default validation honest.

### Task 5: Generated Scaffold Drift Check

- [x] Decide whether `sample-generated-desktop-runtime-material-shader-package` and `tools/new-game.ps1 -Template DesktopRuntimeMaterialShaderPackage` should adopt depth-aware postprocess by default in this slice.
- [x] If adopting, apply the same CLI/report/shader/template changes and update `tools/check-ai-integration.ps1`.
- [x] If deferring, update docs/manifest/guidance to distinguish the hand-authored `sample_desktop_runtime_game` package proof from generated scaffold color-only postprocess.
- [x] Run focused agent/schema checks.

### Task 6: Docs, Manifest, Guidance, Review, and Validation

- [x] Update docs, manifest, skills, subagents, and Claude guidance with honest status: package-visible depth-aware postprocess smoke exists for the selected desktop runtime sample, but exact visual pixels are still RHI readback-proven and broader post effects remain follow-up.
- [x] Move this plan from Active slice to Completed in `docs/superpowers/plans/README.md` after validation evidence is recorded.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`, focused build/CTest, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`, selected package validation, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, cpp-reviewer, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record PASS/blockers and immediately select the next production slice.

## Validation Results

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`: PASS.
- Focused build: `mirakana_runtime_host_sdl3_tests` and `mirakana_runtime_host_sdl3_public_api_compile`: PASS.
- Focused CTest: `mirakana_runtime_host_sdl3_(tests|public_api_compile)`: PASS.
- RED review regression tests verified that D3D12/Vulkan postprocess depth input without postprocess failed before the invalid-request guard, then passed after the fix.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`: PASS after rerunning with elevated permissions for the known sandbox vcpkg 7zip `CreateFileW stdin failed with 5` blocker.
- Selected D3D12 package validation for `sample_desktop_runtime_game`: PASS after rerunning with elevated permissions for the known sandbox vcpkg 7zip blocker. Smoke output included `postprocess_status=ready`, `postprocess_depth_input_requested=1`, and `postprocess_depth_input_ready=1`.
- Selected Vulkan package validation for `sample_desktop_runtime_game -RequireVulkanShaders`: PASS after rerunning with elevated permissions for the known sandbox vcpkg 7zip blocker. Smoke output included `renderer=vulkan`, `postprocess_status=ready`, `postprocess_depth_input_requested=1`, and `postprocess_depth_input_ready=1`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`: diagnostic-only PASS; D3D12 DXIL and Vulkan SPIR-V are ready, Metal `metal` and `metallib` are missing on this Windows host.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: PASS.
- `cpp-reviewer`: review findings around invalid depth-input requests without postprocess and preservation of requested diagnostics were fixed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1`: diagnostic-only PASS; clang-tidy config is valid, but strict analysis is gated by missing `out/build/dev/compile_commands.json` before the final configure generated it.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS. Known host-gated diagnostics remain Metal `metal`/`metallib` missing, Apple packaging blocked by missing macOS/Xcode tools, Android release signing not configured, Android device smoke not connected, and strict clang-tidy analysis dependent on a generated compile database.

## Remaining Follow-Up

- Next selected production slice: package-visible directional shadow receiver proof for the desktop runtime package lane, promoting the existing D3D12/Vulkan shadow receiver readback foundation into a smoke-visible sample contract.
- SSAO, depth of field, fog/volumetrics, temporal history, bloom/HDR/tonemapping, postprocess material authoring, editor controls, GPU markers, full render-graph scheduling, Metal postprocess depth behind Apple host gates, comparison-filtered shadows, cascades, shadow atlases, and broader package-visible shadow quality remain follow-up.

## Done When

- SDL3 desktop presentation can opt into depth-aware postprocess without native handle leakage.
- `sample_desktop_runtime_game` package smoke can require and report depth-aware postprocess readiness.
- D3D12 selected package validation passes on this Windows host, and Vulkan package validation either passes or records exact toolchain/runtime gates.
- Docs, manifest, skills, subagents, Claude guidance, and validation scripts distinguish package-visible smoke readiness from full production postprocess effects.
- Focused validation and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or concrete host blockers are recorded.
