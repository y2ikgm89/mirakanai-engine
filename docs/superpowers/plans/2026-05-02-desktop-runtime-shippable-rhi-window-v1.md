# Desktop Runtime Shippable RHI Window v1 Implementation Plan (2026-05-02)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Move the desktop visible game runtime shell closer to a shippable native RHI-backed game-window presentation by making the committed packaged 2D sample use the same host-supplied D3D12/Vulkan shader-bytecode presentation path already used by the shell and 3D sample.

**Architecture:** Keep gameplay on public `mirakana::GameApp`, `mirakana::IRenderer`, `mirakana_scene`, `mirakana_ui`, and `mirakana_audio`. The 2D sample will load packaged D3D12 DXIL or Vulkan SPIR-V shader artifacts through `mirakana_runtime_host::load_desktop_shader_bytecode_pair`, pass them into `mirakana::SdlDesktopGameHost`, and continue to expose only backend-neutral presentation report fields. CMake package metadata and validation recipes will describe the selected shader artifacts and installed smoke args; no native/RHI handles are exposed to gameplay.

**Tech Stack:** C++23, CMake desktop-runtime targets, SDL3 runtime host, `mirakana_runtime_host_sdl3_presentation`, D3D12 DXIL, Vulkan SPIR-V, PowerShell static/package validation, JSON manifests.

---

## Status

- **Active slice:** selected after `docs/superpowers/plans/2026-05-02-editor-ai-playtest-operator-workflow-ux-v1.md` recorded all tasks complete and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` PASS evidence.
- **Scope boundary:** Windows D3D12 is the primary package-ready native path. Vulkan stays strict host/toolchain/runtime gated. Metal, material/shader graphs, live shader generation, production text/font/IME/accessibility, broad package cooking, broad streaming, native handle exposure, and production renderer quality are out of scope.

## Context

`sample_desktop_runtime_game` already validates the host-owned native RHI scene GPU path with package files, D3D12/Vulkan shader artifacts, postprocess depth input, directional shadow filtering, and native UI overlay fields. `sample_2d_desktop_runtime_package` proves the cooked 2D package gameplay loop but currently only reports deterministic `NullRenderer` fallback. The smallest useful production slice is to reuse the existing shell-style `SdlDesktopPresentationD3d12RendererDesc` / `SdlDesktopPresentationVulkanRendererDesc` path for the 2D sample so packaged 2D and 3D samples both have native game-window smoke selection without changing public gameplay APIs.

## Done When

- [x] The previous Editor AI Playtest Operator Workflow UX active plan is recorded as completed/historical in the plan registry, docs, and manifest.
- [x] `sample_2d_desktop_runtime_package` accepts `--require-d3d12-shaders`, `--require-d3d12-renderer`, `--require-vulkan-shaders`, and `--require-vulkan-renderer`.
- [x] The 2D sample loads package-local D3D12 DXIL and Vulkan SPIR-V artifacts through the existing first-party shader bytecode loader and passes them into `SdlDesktopGameHost` without exposing native handles to gameplay.
- [x] CMake emits 2D sample D3D12/Vulkan shader artifact metadata, builds the artifacts when DXC is available, installs them for the selected package target, and validates D3D12 package smoke through metadata-selected args.
- [x] Game manifests, schema/static checks, `engine/agent/manifest.json`, `docs/current-capabilities.md`, `docs/roadmap.md`, and `docs/superpowers/plans/README.md` describe the new 2D native presentation proof and keep unsupported claims explicit.
- [x] Validation evidence is recorded for `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`, `tools/package-desktop-runtime.ps1 -GameTarget sample_2d_desktop_runtime_package`, `tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, or a concrete host/toolchain blocker is recorded.

## Implementation Tasks

### Task 1: Active Plan Handoff

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `engine/agent/manifest.json`
- Create: `docs/superpowers/plans/2026-05-02-desktop-runtime-shippable-rhi-window-v1.md`

- [x] **Step 1: Record old active completion**

Move `2026-05-02-editor-ai-playtest-operator-workflow-ux-v1.md` from active slice to recent completed slices and summarize that the plan already contains `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` PASS evidence.

- [x] **Step 2: Select this active slice**

Set `aiOperableProductionLoop.currentActivePlan` to this plan and make `recommendedNextPlan` point at post-slice completion review.

### Task 2: RED Static Checks

**Files:**
- Modify: `tools/check-json-contracts.ps1`

- [x] **Step 1: Add static expectations before implementation**

Extend the existing `sample_2d_desktop_runtime_package` checks to require:

```powershell
"--require-d3d12-shaders"
"--require-d3d12-renderer"
"--require-vulkan-shaders"
"--require-vulkan-renderer"
"sample_2d_desktop_runtime_package_sprite.vs.dxil"
"sample_2d_desktop_runtime_package_sprite.ps.dxil"
"sample_2d_desktop_runtime_package_sprite.vs.spv"
"sample_2d_desktop_runtime_package_sprite.ps.spv"
"installed-d3d12-window-smoke"
"installed-vulkan-window-smoke"
```

- [x] **Step 2: Verify RED**

Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.

Expected: FAIL because the 2D sample manifest, CMake metadata, and main program do not yet expose the required native presentation fields.

### Task 3: 2D Native Presentation Implementation

**Files:**
- Modify: `games/sample_2d_desktop_runtime_package/main.cpp`
- Add: `games/sample_2d_desktop_runtime_package/shaders/runtime_2d_sprite.hlsl`
- Modify: `games/CMakeLists.txt`
- Modify: `games/sample_2d_desktop_runtime_package/game.agent.json`
- Modify: `games/sample_2d_desktop_runtime_package/README.md`

- [x] **Step 1: Add shader-backed CLI and host wiring**

Add D3D12/Vulkan shader artifact paths and CLI flags to the 2D sample. Load shader artifacts with `mirakana::load_desktop_shader_bytecode_pair`, construct optional `SdlDesktopPresentationD3d12RendererDesc` / `SdlDesktopPresentationVulkanRendererDesc`, and pass them into `SdlDesktopGameHostDesc`. If `--require-d3d12-renderer` or `--require-vulkan-renderer` is requested and the selected presentation backend does not match, return a non-zero smoke failure after printing presentation diagnostics.

- [x] **Step 2: Add package shader artifacts**

Compile `runtime_2d_sprite.hlsl` to:

```text
shaders/sample_2d_desktop_runtime_package_sprite.vs.dxil
shaders/sample_2d_desktop_runtime_package_sprite.ps.dxil
shaders/sample_2d_desktop_runtime_package_sprite.vs.spv
shaders/sample_2d_desktop_runtime_package_sprite.ps.spv
```

Install D3D12 artifacts when `sample_2d_desktop_runtime_package` is the selected package target. Install Vulkan artifacts only when the selected package requires Vulkan shaders.

- [x] **Step 3: Update 2D manifest recipes**

Add D3D12 and Vulkan installed window smoke validation recipe rows. Keep Vulkan host/toolchain gated and keep public native/RHI handles, production sprite batching, production atlas packing, and renderer quality unsupported.

### Task 4: Docs And Manifest Sync

**Files:**
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.json`
- Modify as needed: `tools/check-ai-integration.ps1`
- Modify as needed: `schemas/game-agent.schema.json`

- [x] **Step 1: Sync capability text**

Document that `2d-desktop-runtime-package` now has a host-gated D3D12 package window smoke using host-owned RHI presentation, while native sprite batching and production renderer quality remain unsupported.

- [x] **Step 2: Sync machine-readable contract**

Update recipes, validation mappings, host gates, sample readiness, and current active plan fields so `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, schema, and agent checks agree.

### Task 5: Validation Evidence

**Files:**
- Modify: this plan file

- [x] **Step 1: Run required validation**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_2d_desktop_runtime_package
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

- [x] **Step 2: Record evidence**

Record PASS output or exact failing command plus host/toolchain blocker in this plan. Do not mark this slice complete without fresh evidence.

## Validation Evidence

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` RED: FAIL as expected before implementation because `sample_2d_desktop_runtime_package/game.agent.json`, `games/CMakeLists.txt`, and `main.cpp` did not yet advertise the required 2D native package window presentation contract.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS (`json-contract-check: ok`).
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS (`ai-integration-check: ok`).
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`: PASS; generated manifest context includes this plan as `aiOperableProductionLoop.currentActivePlan`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`: initial run failed on `sample_2d_desktop_runtime_package_*` tests with `runtime package failure asset=4819170742299961378 path=runtime/assets/2d/level.tilemap: cooked payload hash mismatch`; fixed `games/sample_2d_desktop_runtime_package/runtime/sample_2d_desktop_runtime_package.geindex` to match the committed tilemap payload, then re-ran PASS with `100% tests passed, 0 tests failed out of 16`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_2d_desktop_runtime_package`: PASS. Installed smoke reported `renderer=d3d12`, `presentation_selected=d3d12`, `presentation_fallback=none`, `presentation_used_null_fallback=0`, and `presentation_backend_report=d3d12:ready:none: D3D12 renderer ready.`
- `pwsh -NoProfile -ExecutionPolicy Bypass -Command "& .\tools\package-desktop-runtime.ps1 -GameTarget sample_2d_desktop_runtime_package -RequireVulkanShaders -SmokeArgs @('--smoke','--require-config','runtime/sample_2d_desktop_runtime_package.config','--require-scene-package','runtime/sample_2d_desktop_runtime_package.geindex','--require-vulkan-shaders','--video-driver','windows','--require-vulkan-renderer')"`: PASS. Installed smoke reported `renderer=vulkan`, `presentation_selected=vulkan`, `presentation_fallback=none`, `presentation_used_null_fallback=0`, and `presentation_backend_report=vulkan:ready:none: Vulkan renderer ready.`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game`: PASS. Installed smoke reported `renderer=d3d12`, `scene_gpu_status=ready`, `postprocess_status=ready`, `postprocess_depth_input_ready=1`, `directional_shadow_status=ready`, `directional_shadow_ready=1`, `ui_overlay_status=ready`, `ui_texture_overlay_status=ready`, and `framegraph_passes=3`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS (`validate: ok`, 28/28 dev tests passed). Diagnostic-only host gates remain for Metal/macOS/iOS tooling and non-strict tidy compile database availability; they are outside this slice's ready claim.
