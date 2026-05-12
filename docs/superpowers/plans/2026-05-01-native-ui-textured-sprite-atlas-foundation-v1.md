# Native UI Textured Sprite Atlas Foundation v1 Implementation Plan (2026-05-01)

> **For agentic workers:** Use this as the next focused C-phase slice after `native-gpu-runtime-ui-overlay`. Do not append this work to the completed native overlay, 2D, 3D, renderer/RHI, or desktop runtime plans.

**Goal:** Extend the renderer-owned native UI overlay proof from colored box/image-placeholder sprites to a narrow cooked-texture UI image sprite path, using first-party package assets and backend-neutral atlas metadata while keeping source image decoding, production atlas packing, font glyph atlases, text shaping, IME, OS accessibility bridges, and public native/RHI handles out of gameplay-facing APIs.

**Architecture:** Keep `mirakana_ui` renderer/RHI/platform independent. Keep `mirakana_ui_renderer` as the public adapter that resolves UI image payloads into backend-neutral sprite intents. Add renderer/runtime-host/package plumbing only behind `mirakana_renderer` and `engine/runtime_host/sdl3` so D3D12 and strict Vulkan can bind a renderer-owned sampled UI atlas page for the final overlay pass without exposing SDL3, Win32, D3D12, Vulkan, Metal, Dear ImGui, or RHI handles.

**Tech Stack:** C++23, `mirakana_ui`, `mirakana_ui_renderer`, `mirakana_renderer`, `mirakana_runtime_scene_rhi` or a narrower renderer-owned cooked texture binding path, `mirakana_runtime_host_sdl3_presentation`, `sample_desktop_runtime_game`, D3D12 DXIL artifacts, strict Vulkan SPIR-V artifacts, manifest/static checks, desktop runtime package validation.

---

## Goal

Build the smallest truthful textured UI sprite foundation that an AI agent can select and validate:

- package-backed UI image sprite submission over the existing `native-gpu-runtime-ui-overlay` path
- renderer-owned sampled atlas page or single-page atlas binding for cooked first-party texture payloads
- deterministic status fields proving texture overlay requests, atlas page readiness, sprites submitted, texture binds, and draw counts
- D3D12 selected package proof and strict Vulkan selected package proof on ready hosts
- honest fallback diagnostics when native texture UI overlay is unavailable
- docs/manifest/checks that keep source image decoding, production texture atlas packing, text shaping, font rasterization, glyph atlases, IME, accessibility bridges, Metal readiness, and general production UI renderer quality planned

## Context

- `native-gpu-runtime-ui-overlay` now draws first-party `mirakana_ui_renderer` box/image-placeholder sprite submissions as a renderer-owned final native overlay on the sample desktop runtime scene/postprocess/shadow package path.
- That proof is color-only. It intentionally does not decode images, allocate real atlases, bind sampled UI textures, or claim production image quality.
- Existing cooked runtime texture payloads and runtime scene GPU upload paths already prove backend-private sampled texture binding for scene materials.
- The next useful slice is a narrow UI-specific texture submission contract, not production image loading or text/font work.

## Constraints

- Do not add third-party dependencies in this slice.
- Do not parse PNG/JPEG/glTF/source assets at runtime.
- Do not expose SDL3, Win32, D3D12, Vulkan, Metal, Dear ImGui, `IRhiDevice`, descriptor sets, shader modules, swapchains, or native GPU handles through gameplay-facing APIs.
- Do not make `mirakana_ui` depend on `mirakana_renderer`, `mirakana_rhi`, platform, SDL3, Dear ImGui, or backend APIs.
- Do not claim production font/text shaping, bidirectional text, glyph atlases, image decoding, automatic atlas packing, IME, OS accessibility bridges, material graph/shader graph, package streaming, skeletal animation, GPU skinning, Metal readiness, editor productization, or general production renderer quality.
- Keep Vulkan strict and Metal/Apple host-gated.

## Done When

- Focused RED -> GREEN evidence is recorded here.
- The manifest exposes the textured UI sprite/atlas capability honestly as ready, host-gated, planned, or blocked according to validation evidence.
- Static checks reject stale claims that source image decoding, production atlas packing, production text/font, public native handles, or general production UI renderer quality are ready without evidence.
- Focused source-tree and selected package validation recipes pass, or concrete host/toolchain blockers are recorded.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`, selected D3D12/Vulkan package validation, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record blockers.

## Implementation Tasks

### Task 1: Inventory And Contract Selection

**Files:**
- Read: `engine/ui/include/mirakana/ui/ui.hpp`
- Read: `engine/ui_renderer/`
- Read: `engine/renderer/src/rhi_native_ui_overlay.*`
- Read: `engine/renderer/src/rhi_postprocess_frame_renderer.cpp`
- Read: `engine/renderer/src/rhi_directional_shadow_smoke_frame_renderer.cpp`
- Read: `engine/runtime_scene_rhi/`
- Read: `engine/runtime_host/sdl3/`
- Read: `games/sample_desktop_runtime_game/`
- Read: `engine/agent/manifest.json`

- [x] Decide whether atlas metadata belongs in `mirakana_ui_renderer`, `mirakana_renderer`, or the sample package contract.
- [x] Decide the capability/recipe id before editing production code.
- [x] Identify the minimum D3D12/Vulkan shader artifacts and package status fields required.
- [x] Record whether this is source-tree-ready, host-gated desktop-package-only, or still planned.

### Task 2: RED Checks And Tests

**Files:**
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `tools/validate-installed-desktop-runtime.ps1`
- Modify: focused renderer/runtime-host tests or `games/sample_desktop_runtime_game`

- [x] Add failing static checks for the selected textured UI capability id, validation recipes, unsupported claims, and sample status fields.
- [x] Add a failing behavior proof that a resolved UI image sprite is counted separately from color-only overlay boxes/placeholders.
- [x] Add a strict Vulkan host-gated proof or explicit blocker path for the same fields.
- [x] Record RED evidence in this plan with the exact failing command and diagnostic line.

### Task 3: Textured UI Overlay Implementation

**Files:**
- Modify only the renderer/runtime-host/sample modules selected in Task 1.
- Add public headers only if existing public `mirakana::` APIs cannot express the selected proof cleanly.

- [x] Implement renderer-owned sampled UI sprite binding for cooked texture/atlas metadata.
- [x] Preserve deterministic colored placeholder overlay behavior when no texture binding is available.
- [x] Keep backend resources, descriptors, shader artifacts, command lists, swapchains, and native handles private.
- [x] Keep source image decoding, atlas packing, text/font, IME, and accessibility bridge work unavailable unless a separate adapter-backed slice implements it.

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

- [x] Promote only the validated textured UI capability; keep production image decoding/atlas packing/text/font/accessibility/general renderer claims planned.
- [x] Update docs and prompts so agents can distinguish colored overlay readiness from textured UI sprite readiness.
- [x] Run focused checks and default validation.
- [x] Record evidence, mark this plan complete, and create the next dated focused plan.

## Validation Evidence

Record command results here while implementing this plan.

- RED static check: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed before implementation with `engine/agent/manifest.json aiOperableProductionLoop missing recipe id: native-ui-textured-sprite-atlas`.
- RED schema check: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` failed before implementation with `engine manifest aiOperableProductionLoop missing recipe id: native-ui-textured-sprite-atlas`.
- RED compile blocker: `cmake --build --preset dev --target mirakana_ui_renderer_tests` and `cmake --build --preset dev --target mirakana_renderer_tests` could not run in this PowerShell environment because `cmake` is not on `PATH`. The focused tests added in `tests/unit/ui_renderer_tests.cpp` and `tests/unit/renderer_rhi_tests.cpp` still require the production `SpriteUvRect`, `SpriteTextureRegion`, `NativeUiOverlayAtlasBinding`, atlas-ready renderer APIs, and textured overlay stats.
- GREEN schema check: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed with `json-contract-check: ok` after adding the `native-ui-textured-sprite-atlas` recipe, validation recipes, sample smoke fields, and unsupported-claim checks.
- GREEN agent check: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed with `ai-integration-check: ok` after synchronizing manifest/docs/tool checks and sample game status fields.
- GREEN agent context: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` passed and exposed `mirakana_ui_renderer` atlas metadata, `native-ui-textured-sprite-atlas`, and the textured UI validation recipes.
- GREEN API boundary: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed after adding backend-neutral `SpriteUvRect`, `SpriteTextureRegion`, `NativeUiOverlayAtlasBinding`, and renderer stats without exposing native handles.
- GREEN shader toolchain diagnostic: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1` passed as diagnostic-only with D3D12 DXIL and Vulkan SPIR-V ready; Metal `metal`/`metallib` remain missing host blockers.
- GREEN desktop runtime: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` passed, `100% tests passed, 0 tests failed out of 14`.
- GREEN D3D12 package: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game` passed with `renderer=d3d12`, `ui_texture_overlay_status=ready`, `ui_texture_overlay_atlas_ready=1`, `ui_texture_overlay_sprites_submitted=2`, `ui_texture_overlay_texture_binds=2`, and `ui_texture_overlay_draws=2`.
- GREEN strict Vulkan package: `pwsh -NoProfile -ExecutionPolicy Bypass -Command "& .\tools\package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game -RequireVulkanShaders -SmokeArgs @('--smoke','--require-config','runtime/sample_desktop_runtime_game.config','--require-scene-package','runtime/sample_desktop_runtime_game.geindex','--require-vulkan-scene-shaders','--video-driver','windows','--require-vulkan-renderer','--require-scene-gpu-bindings','--require-postprocess','--require-postprocess-depth-input','--require-directional-shadow','--require-directional-shadow-filtering','--require-native-ui-overlay','--require-native-ui-textured-sprite-atlas')"` passed with `renderer=vulkan`, `framegraph_passes=3`, `ui_texture_overlay_status=ready`, `ui_texture_overlay_atlas_ready=1`, `ui_texture_overlay_sprites_submitted=2`, `ui_texture_overlay_texture_binds=2`, and `ui_texture_overlay_draws=2`.
- GREEN default validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with `validate: ok` and `100% tests passed, 0 tests failed out of 28`. Diagnostic-only blockers remain unchanged: Metal `metal`/`metallib` missing, Apple packaging requires macOS/Xcode, Android release signing/device smoke not fully configured, and tidy strict analysis needs a compile database.
