# Native GPU UI Overlay Foundation v1 Implementation Plan (2026-05-01)

> **For agentic workers:** Use this as the next focused C-phase slice after `3d-playable-desktop-package`. Do not append this work to completed 2D, 3D, renderer/RHI, or desktop runtime plans.

**Goal:** Turn first-party runtime UI box/image placeholder submissions from diagnostics-only counters into a narrow native GPU overlay path for desktop runtime scene packages, while keeping gameplay on `mirakana_ui` / `mirakana_ui_renderer` / `mirakana::IRenderer` and keeping SDL3, Win32, D3D12, Vulkan, Metal, Dear ImGui, and RHI handles private.

**Architecture:** Extend the existing renderer-owned scene/postprocess/shadow frame renderers so UI sprites submitted through public `IRenderer::draw_sprite` can be drawn as a final overlay pass when the native backend path is ready. Keep `mirakana_ui` renderer-independent and keep `mirakana_ui_renderer` as the public adapter that emits backend-neutral sprite commands. Use D3D12 as the primary Windows proof and Vulkan as a strict host/toolchain/runtime-gated parity lane.

**Tech Stack:** C++23, `mirakana_renderer`, `mirakana_ui`, `mirakana_ui_renderer`, `mirakana_runtime_host_sdl3_presentation`, `engine/agent/manifest.json`, schema/static checks, desktop runtime package validation, D3D12 DXIL artifacts, strict Vulkan SPIR-V artifacts.

---

## Goal

Build the smallest truthful runtime UI overlay foundation that an AI agent can select and validate:

- renderer-owned native GPU submission for `mirakana_ui_renderer` box/image placeholder sprites over the current scene/postprocess/shadow package path
- deterministic status fields proving overlay requests, submitted box count, native overlay readiness, and native overlay draw count
- D3D12 selected package proof and strict Vulkan selected package proof on ready hosts
- `NullRenderer` and unsupported-backend fallback diagnostics that keep HUD counters deterministic when native overlay drawing is unavailable
- docs/manifest/checks that keep production text shaping, font rasterization, glyph atlases, image decoding, texture atlases, IME, accessibility OS bridges, material graph, package streaming, and general production renderer quality planned

## Context

- `3d-playable-desktop-package` is host-gated and validated for `sample_desktop_runtime_game`.
- That recipe intentionally counts HUD submissions only. Native GPU HUD or sprite overlay output is unsupported because the 3D scene/postprocess renderers previously replayed UI sprites through the scene material pipeline.
- `mirakana_ui` already builds first-party renderer submission payloads without depending on renderer, SDL3, Dear ImGui, OS, or GPU APIs.
- `mirakana_ui_renderer` already maps UI boxes and image placeholders to backend-neutral `SpriteCommand` rows through public `IRenderer`.
- Current renderer-owned RHI paths need an explicit overlay pass or overlay-safe sprite path before UI can be claimed visible on native GPU scene packages.

## Constraints

- Do not add third-party dependencies in this slice.
- Do not expose SDL3, Win32, D3D12, Vulkan, Metal, Dear ImGui, `IRhiDevice`, descriptor sets, shader modules, swapchains, or native GPU handles through gameplay-facing APIs.
- Do not make `mirakana_ui` depend on `mirakana_renderer`, `mirakana_rhi`, platform, SDL3, Dear ImGui, or backend APIs.
- Do not claim production font/text shaping, bidirectional text, glyph atlases, image decoding, texture atlas packing, IME, OS accessibility bridges, material graph/shader graph, package streaming, skeletal animation, GPU skinning, Metal readiness, editor productization, or general production renderer quality.
- Keep Vulkan strict and Metal/Apple host-gated. D3D12 can be primary only where package validation proves it on the current Windows host.

## Done When

- Focused RED -> GREEN evidence is recorded here.
- The manifest exposes the new overlay capability honestly as ready, host-gated, planned, or blocked according to validation evidence.
- Static checks reject stale claims that native GPU runtime UI overlay, production text/font/image rendering, public native handles, or general production renderer quality are ready without evidence.
- Focused source-tree and selected package validation recipes pass, or concrete host/toolchain blockers are recorded.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`, selected package validation, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record blockers.

## Implementation Tasks

### Task 1: Inventory And Overlay Boundary Selection

**Files:**
- Read: `engine/renderer/include/mirakana/renderer/renderer.hpp`
- Read: `engine/renderer/src/rhi_frame_renderer.cpp`
- Read: `engine/renderer/src/rhi_postprocess_frame_renderer.cpp`
- Read: `engine/renderer/src/rhi_directional_shadow_smoke_frame_renderer.cpp`
- Read: `engine/ui_renderer/`
- Read: `engine/runtime_host/sdl3/`
- Read: `games/sample_desktop_runtime_game/`
- Read: `engine/agent/manifest.json`

- [x] Decide whether the first overlay proof belongs in `RhiPostprocessFrameRenderer`, `RhiDirectionalShadowSmokeFrameRenderer`, a shared renderer overlay helper, or host-owned composition.
- [x] Decide the capability/recipe id before editing production code.
- [x] Identify the minimum shader artifacts and package validation fields required for D3D12 and strict Vulkan proofs.
- [x] Record whether this is source-tree-ready, host-gated desktop-package-only, or still planned.

### Task 2: RED Checks And Tests

**Files:**
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `engine/agent/manifest.json`
- Modify: focused renderer/runtime-host tests or `games/sample_desktop_runtime_game`

- [x] Add failing static checks for the selected overlay capability id, status, validation recipes, unsupported claims, and sample status fields.
- [x] Add a failing behavior proof that native D3D12 overlay output is requested and counted separately from scene mesh/postprocess/shadow draws.
- [x] Add a strict Vulkan host-gated proof or explicit blocker path for the same fields.
- [x] Record RED evidence in this plan with the exact failing command and diagnostic line.

### Task 3: Overlay Implementation

**Files:**
- Modify only the renderer/runtime-host/sample modules selected in Task 1.
- Add public headers only if existing public `mirakana::` APIs cannot express the selected overlay proof cleanly.

- [x] Implement an overlay-safe sprite path for `mirakana_ui_renderer` submissions after scene/postprocess/shadow rendering.
- [x] Preserve deterministic `NullRenderer` fallback counters and diagnostics when native overlay drawing is unavailable.
- [x] Keep backend resources, pipeline layouts, descriptors, shader artifacts, command lists, swapchains, and native handles private.
- [x] Keep UI text/font/image decoding unavailable unless a separate adapter-backed slice implements it.

### Task 4: Manifest, Docs, Checks, And Validation

**Files:**
- Modify: `engine/agent/manifest.json`
- Modify: `schemas/engine-agent.schema.json` if capability shape changes
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `tools/agent-context.ps1` if top-level output needs new fields
- Modify: `docs/ai-game-development.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `docs/specs/game-prompt-pack.md`
- Modify: `docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`

- [x] Promote only the validated overlay capability; keep production UI/text/font/image/atlas/accessibility and general production renderer claims planned.
- [x] Update docs and prompts so agents can distinguish HUD diagnostics counters from native GPU runtime UI overlay readiness.
- [x] Run focused checks and default validation.
- [x] Record evidence, mark this plan complete, and create the next dated focused plan.

## Validation Evidence

Record command results here while implementing this plan.

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed after static checks were tightened, as expected, with `engine/agent/manifest.json aiOperableProductionLoop missing recipe id: native-gpu-runtime-ui-overlay`.
- RED compile blocker: `cmake --build --preset dev --target mirakana_renderer_tests` could not run in the current PowerShell environment because `cmake` is not on `PATH`. The newly added renderer/API tests in `tests/unit/renderer_rhi_tests.cpp` and `tests/unit/runtime_host_sdl3_public_api_compile.cpp` still require production fields/methods such as `enable_native_ui_overlay`, `native_ui_overlay_ready()`, and `RendererStats::native_ui_overlay_draws`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after adding the `native-gpu-runtime-ui-overlay` recipe, sample smoke fields, unsupported-claim checks, and validation recipe mapping.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed after the manifest/schema/static-contract updates.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` passed, 14/14, with the native overlay source-tree sample path included in `sample_desktop_runtime_game_smoke`.
- GREEN: `tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game` passed for the D3D12 selected package lane. Installed smoke reported `renderer=d3d12`, `scene_gpu_status=ready`, `postprocess_status=ready`, `directional_shadow_status=ready`, `ui_overlay_requested=1`, `ui_overlay_status=ready`, `ui_overlay_ready=1`, `ui_overlay_sprites_submitted=2`, and `ui_overlay_draws=2`.
- RED: `tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game -RequireVulkanShaders -SmokeArgs @('--smoke','--require-config','runtime/sample_desktop_runtime_game.config','--require-scene-package','runtime/sample_desktop_runtime_game.geindex','--require-vulkan-scene-shaders','--video-driver','windows','--require-vulkan-renderer','--require-scene-gpu-bindings','--require-postprocess','--require-postprocess-depth-input','--require-directional-shadow','--require-directional-shadow-filtering','--require-native-ui-overlay')` initially failed during installed smoke with `Command failed with exit code -1073740791`. Root cause was the Vulkan RHI command bridge recording only one dynamic-rendering draw per RHI render pass and not forwarding a bound vertex buffer for non-indexed `draw()`, while the native UI overlay draws a vertex-buffer-backed sprite pass after the postprocess draw.
- GREEN: the Vulkan RHI command bridge now records later draw calls in the same RHI render pass with `LoadAction::load` and forwards optional bound vertex buffers for non-indexed draws. `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` passed again, 14/14.
- GREEN: the strict Vulkan selected package command above passed. Installed smoke reported `renderer=vulkan`, `scene_gpu_status=ready`, `postprocess_status=ready`, `postprocess_depth_input_ready=1`, `directional_shadow_status=ready`, `directional_shadow_ready=1`, `directional_shadow_filter_mode=fixed_pcf_3x3`, `framegraph_passes=3`, `ui_overlay_requested=1`, `ui_overlay_status=ready`, `ui_overlay_ready=1`, `ui_overlay_sprites_submitted=2`, and `ui_overlay_draws=2`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with the existing diagnostic-only host gates recorded by those tools.

## Completion

Completed on 2026-05-01. The next focused plan is [Native UI Textured Sprite Atlas Foundation v1](2026-05-01-native-ui-textured-sprite-atlas-foundation-v1.md).
