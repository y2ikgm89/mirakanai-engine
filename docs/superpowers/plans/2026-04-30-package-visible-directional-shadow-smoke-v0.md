# Package-Visible Directional Shadow Smoke v0 Implementation Plan (2026-04-30)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Promote the existing directional shadow receiver readback proof into a package-visible SDL3 desktop runtime smoke that can request, report, and validate one deterministic shadowed receiver path in `sample_desktop_runtime_game`.

**Architecture:** Keep shadow map textures, depth views, sampled views, samplers, descriptor bindings, pipeline state, and native backend objects owned by `mirakana_renderer`, `mirakana_runtime_host_sdl3_presentation`, and private RHI backends. Expose only first-party request flags, readiness booleans, diagnostics, and renderer stats through runtime-host/sample status output. The sample may bind first-party shadow shader artifacts, but gameplay/public APIs must not receive SDL3, OS, D3D12, Vulkan, Metal, `IRhiDevice`, descriptor, texture, image, or swapchain handles.

**Tech Stack:** C++23, SDL3 desktop runtime host adapter, `mirakana_renderer` shadow plans, D3D12 DXIL package lane, Vulkan SPIR-V package lane when toolchain/runtime ready, first-party HLSL sample shaders, PowerShell validation scripts.

---

## Context

- `mirakana_renderer` already has a backend-neutral directional shadow receiver plan and D3D12/Vulkan readback proof.
- `sample_desktop_runtime_game` now proves scene GPU bindings plus depth-aware postprocess in installed D3D12/Vulkan package smokes.
- Current remaining production gap is that package/runtime users still cannot require visible shadow readiness through the desktop runtime package lane.
- This slice should provide a narrow smoke-visible contract, not a full cascaded shadow implementation.

## Constraints

- Keep public API under `mirakana::`; do not expose SDL3/native/GPU/backend/editor handles to game public APIs.
- Do not add third-party dependencies.
- Do not claim PCF, comparison filtering, cascades, atlases, contact shadows, editor shadow controls, or Metal shadow presentation.
- Public runtime-host header changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- Renderer/RHI/shader/package changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`.
- Desktop runtime/package changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` and selected `tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game`.
- End the slice with focused validation, cpp-reviewer, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

## Tasks

### Task 1: Shadow Smoke Contract Tests

- [x] Add runtime-host tests for a first-party shadow smoke request/report path that preserves honest unavailable diagnostics on dummy/native-fallback surfaces.
- [x] Add public API compile coverage for new request/report fields without backend handles if a public runtime-host contract change is required.
- [x] Verify RED behavior before implementation.

### Task 2: Runtime-Host Shadow Wiring

- [x] Add the minimal first-party request/readiness/report fields for package-visible directional shadow smoke.
- [x] Wire D3D12/Vulkan scene renderer creation to request one shadow receiver path only when the sample opts in.
- [x] Preserve honest `NullRenderer`, missing shader, unsupported backend, and creation-failure diagnostics.
- [x] Re-run focused runtime-host tests until they pass.

### Task 3: Sample Shader and Package Smoke

- [x] Add or extend sample shader artifacts so `sample_desktop_runtime_game` can prove a deterministic shadowed receiver path in package smoke.
- [x] Add `--require-directional-shadow` or equivalent sample smoke flag and status output.
- [x] Update `games/CMakeLists.txt` and `tools/validate-installed-desktop-runtime.ps1` to require the shadow readiness field on selected package lanes.
- [x] Run selected D3D12 package validation.
- [x] Run selected Vulkan package validation with `-RequireVulkanShaders` when the host remains ready; otherwise record exact gates.

### Task 4: Docs, Manifest, Guidance, Review, and Validation

- [x] Update docs, gap analysis, roadmap, manifest, skills, subagents, and Claude guidance with honest scope.
- [x] Move this plan from Active slice to Completed in `docs/superpowers/plans/README.md` after evidence is recorded.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`, focused build/CTest, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`, selected package validation, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, cpp-reviewer, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

## Validation Results

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`: PASS.
- `cmake --preset dev`: PASS.
- Focused dev build: `mirakana_renderer_tests` and `mirakana_runtime_scene_rhi_tests`: PASS.
- Focused dev CTest: `mirakana_renderer_tests|mirakana_runtime_scene_rhi_tests`: PASS.
- Focused desktop-runtime build: `mirakana_runtime_host_sdl3_tests`, `mirakana_runtime_host_sdl3_public_api_compile`, and `sample_desktop_runtime_game`: PASS.
- Focused desktop-runtime CTest: `mirakana_runtime_host_sdl3_tests|mirakana_runtime_host_sdl3_public_api_compile`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`: PASS after rerunning outside the sandbox because vcpkg 7zip extraction hit `CreateFileW stdin failed with 5` in the sandbox.
- Selected D3D12 package validation: `tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game`: PASS with `directional_shadow_status=ready`, `directional_shadow_ready=1`, and `framegraph_passes=3`.
- Selected Vulkan package validation: `tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game -RequireVulkanShaders ... --require-directional-shadow`: PASS with `presentation_selected=vulkan`, `directional_shadow_ready=1`, and `framegraph_passes=3`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS after syncing manifest/game manifest/gap-analysis/skills/subagent guidance.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`: diagnostic-only with D3D12 DXIL ready, Vulkan SPIR-V ready, and known Metal `metal` / `metallib` missing blockers.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1`: config PASS; strict analysis remains diagnostic-only because the Visual Studio generator did not emit `out/build/dev/compile_commands.json`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS with known diagnostic-only Metal/Apple/signing/device/tidy host gates.

## Remaining Follow-Up

- PCF/comparison filtering, cascades, atlases, shadow settings authoring, editor controls, terrain/large-scene shadow policies, Metal shadow presentation behind Apple host gates, and GPU timing/markers.

## Done When

- `sample_desktop_runtime_game` can opt into and report a package-visible directional shadow smoke path without native handle leakage.
- Selected D3D12 package validation passes on this Windows host, and selected Vulkan package validation either passes or records exact host/toolchain gates.
- Docs, manifest, skills, subagents, Claude guidance, and validation scripts distinguish this smoke contract from full production shadow quality.
- Focused validation and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or concrete host blockers are recorded.
