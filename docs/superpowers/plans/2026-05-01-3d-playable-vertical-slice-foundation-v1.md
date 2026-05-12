# 3D Playable Vertical Slice Foundation v1 Implementation Plan (2026-05-01)

> **For agentic workers:** Use this as the next focused C-phase slice after `2d-desktop-runtime-package`. Do not append this work to completed MVP, renderer/RHI foundation, 2D source-tree, or 2D desktop package plans.

**Goal:** Promote the planned 3D generated-game path into an honest first 3D playable recipe using cooked packages, first-party scene/material/runtime contracts, and host-gated desktop runtime validation without exposing native or RHI handles to gameplay code.

**Architecture:** Start from existing cooked-scene/material package and desktop runtime host paths, then define the smallest AI-selectable 3D recipe boundary. Gameplay stays on public `mirakana::GameApp`, `mirakana_scene`, `mirakana_scene_renderer`, `mirakana_runtime`, `mirakana_ui`, `mirakana_audio`, and renderer contracts. D3D12/Vulkan/Metal devices, shader artifacts, swapchains, scene GPU binding palettes, and native handles remain owned by renderer/runtime-host/RHI adapters.

**Tech Stack:** C++23, CMake desktop runtime target metadata, existing first-party cooked package formats, `engine/agent/manifest.json`, schema/static checks, docs, PowerShell 7 `tools/*.ps1` validation commands, D3D12 primary Windows package lane, strict Vulkan host/toolchain/runtime gates.

---

## Goal

Build the smallest truthful 3D vertical-slice foundation that an AI agent can select and validate:

- a non-`future-*` 3D production-loop recipe only after its behavior is proven
- a sample or focused target that composes static mesh scene state, material instance intent, camera/controller movement, light/shadow/postprocess package metadata, HUD diagnostics, and deterministic validation output
- package validation that proves cooked config, `.geindex`, mesh, texture, material, scene, and required shader artifacts only at the level actually used by the chosen target
- checks that keep glTF source parsing, skeletal animation, GPU skinning, material graph, broad package streaming, editor productization, native handles, and general production renderer quality planned or host-gated

## Context

- `sample_desktop_runtime_game` already proves a package-visible Lit Material v0 mesh/material scene path, D3D12 and strict Vulkan scene GPU binding on ready hosts, depth-aware postprocess, and directional shadow filtering.
- `desktop-runtime-material-shader-package` is host-gated and scaffold-oriented, not yet a normal generated-game 3D recipe.
- `future-3d-playable-vertical-slice` remains planned in `engine/agent/manifest.json`.
- Scene/Component/Prefab Schema v2, Asset Identity v2, Runtime Resource v2, Renderer/RHI Resource Foundation v1, Frame Graph v1, Upload/Staging v1, AI Command Surface Foundation v1, and 2D slices are completed foundations, not full 3D production readiness.
- Source asset parsing belongs to tools/editor. Runtime gameplay consumes cooked packages and public `mirakana::` APIs.

## Constraints

- Do not add third-party dependencies in this slice.
- Do not parse glTF, PNG, common audio, shader source, or source scene files in runtime gameplay.
- Do not expose SDL3, Win32, D3D12, Vulkan, Metal, Dear ImGui, shader compiler process handles, `IRhiDevice`, backend handles, or native GPU resource handles through gameplay-facing APIs.
- Do not mark skeletal animation, GPU skinning, glTF skin/animation import, material graph, shader graph, live shader generation, broad package streaming, ECS scheduling, editor productization, or production renderer quality as complete.
- Keep Vulkan strict and Metal/Apple host-gated. D3D12 can be primary only where the package validation command proves it on the current Windows host.

## Done When

- Focused RED -> GREEN evidence is recorded here.
- The selected 3D recipe is represented honestly in `engine/agent/manifest.json` as ready, host-gated, or planned according to validation evidence.
- Static checks reject stale `future-3d-playable-vertical-slice` ready claims and reject docs/manifests that imply source asset runtime parsing, public native/RHI handle access, or unvalidated production renderer readiness.
- Focused source-tree and selected desktop package validation recipes pass, or concrete host/toolchain blockers are recorded.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` if public headers change, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`, selected package validation, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record blockers.

## Implementation Tasks

### Task 1: Inventory And Boundary Selection

**Files:**
- Read: `engine/agent/manifest.json`
- Read: `docs/ai-game-development.md`
- Read: `docs/specs/generated-game-validation-scenarios.md`
- Read: `docs/specs/game-prompt-pack.md`
- Read: `games/sample_desktop_runtime_game/`
- Read: `games/sample-generated-desktop-runtime-material-shader-package/`
- Read: `games/CMakeLists.txt`
- Read: `engine/runtime_scene_rhi/`
- Read: `engine/runtime_host/sdl3/`
- Read: `engine/scene_renderer/`

- [x] Decide whether the first 3D proof is a new sample, a narrowed recipe around `sample_desktop_runtime_game`, or a generated-game scaffold hardening step.
- [x] Decide the recipe id before editing production code. Prefer a non-`future-*` id such as `3d-playable-desktop-package` only if validation can prove the selected scope.
- [x] Identify the minimum package files and shader artifacts required by the selected 3D proof.
- [x] Record whether the proof is source-tree-ready, host-gated desktop-package-only, or still planned.

Selected boundary: use the existing `sample_desktop_runtime_game` as the first 3D proof and expose it through the new host-gated recipe id `3d-playable-desktop-package`. The proof is not a broad generated-game production renderer claim. It is a desktop package foundation with cooked config, `.geindex`, texture, mesh, material, scene, target-specific D3D12 scene/postprocess/shadow DXIL artifacts, strict host/toolchain/runtime-gated Vulkan SPIR-V artifacts, static mesh scene submission, material instance intent, camera/controller status, light/shadow/postprocess metadata, HUD diagnostics, and deterministic smoke output. `future-3d-playable-vertical-slice` stays planned for the broader production 3D path.

### Task 2: RED Checks And Tests

**Files:**
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `engine/agent/manifest.json`
- Modify: focused tests under `tests/unit` or a registered sample under `games/`

- [x] Add failing static checks for the selected 3D recipe id, status, required modules, validation recipes, allowed package targets, and unsupported claims.
- [x] Add a failing behavior proof for the selected 3D scene/camera/material/light/package path. If the proof is a sample, register a finite smoke target that reports stable status fields.
- [x] Add stale-claim checks that reject `future-3d-playable-vertical-slice` as ready and reject public native/RHI handles, runtime source parsing, material graph, skeletal/GPU skinning, package streaming, and general production renderer claims.
- [x] Record RED evidence in this plan with the exact failing command and diagnostic line.

### Task 3: 3D Playable Foundation Implementation

**Files:**
- Modify only the modules and samples selected in Task 1.
- Add public headers only if existing public `mirakana::` APIs cannot express the selected 3D proof cleanly.

- [x] Implement the minimum 3D gameplay/sample path selected in Task 1.
- [x] Keep game code on `mirakana::GameApp`, cooked package/runtime APIs, scene/render/UI/audio public contracts, and `mirakana::IRenderer`.
- [x] Keep D3D12/Vulkan/Metal presentation, scene GPU binding, shader artifacts, and backend handles inside runtime-host/renderer/RHI adapters.
- [x] Preserve deterministic `NullRenderer` fallback diagnostics when native presentation or shader artifacts are unavailable.

### Task 4: Manifest, Docs, Checks, And Validation

**Files:**
- Modify: `engine/agent/manifest.json`
- Modify: `schemas/engine-agent.schema.json` if recipe shape changes
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `tools/agent-context.ps1` if top-level output needs new fields
- Modify: `docs/ai-game-development.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `docs/specs/game-prompt-pack.md`
- Modify: `docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`

- [x] Promote only the validated 3D capability; keep glTF import, skeletal animation, GPU skinning, material graph, shader graph, package streaming, editor, Metal, and production renderer claims planned or host-gated.
- [x] Update docs and prompts so agents can distinguish 2D source-tree, 2D desktop package, generated desktop material/shader scaffold, and 3D playable package recipes.
- [x] Run focused checks and default validation.
- [x] Record evidence, mark this plan complete, and create the next dated focused plan.

## Validation Evidence

Record command results here while implementing this plan.

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed after adding `3d-playable-desktop-package` contract checks: `engine/agent/manifest.json aiOperableProductionLoop missing recipe id: 3d-playable-desktop-package`.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` failed after adding matching JSON contract checks: `engine manifest aiOperableProductionLoop missing recipe id: 3d-playable-desktop-package`.
- GREEN 2026-05-01: selected `games/sample_desktop_runtime_game` as the first host-gated 3D proof and added the `3d-playable-desktop-package` recipe around cooked config, `.geindex`, texture, mesh, material, `GameEngine.Scene.v1`, static mesh submission, material instance intent, primary camera/controller status, HUD diagnostics counters, target-specific D3D12 artifacts, strict Vulkan gated artifacts, postprocess, and directional shadow metadata.
- GREEN 2026-05-01: updated `games/sample_desktop_runtime_game` to load a cooked primary camera node, tick deterministic public-input camera/controller movement, rebuild the scene render packet, submit first-party `mirakana_ui` HUD diagnostics through `mirakana_ui_renderer`, and emit stable smoke fields: `camera_primary=1`, `camera_controller_ticks=2`, `camera_final_x=2`, and `hud_boxes=2`.
- GREEN 2026-05-01: synchronized `engine/agent/manifest.json`, `games/sample_desktop_runtime_game/game.agent.json`, `tools/check-ai-integration.ps1`, `tools/check-json-contracts.ps1`, `tools/validate-installed-desktop-runtime.ps1`, and generated-game docs so the new recipe is host-gated and keeps runtime source parsing, material/shader graph, skeletal animation, GPU skinning, package streaming, Metal, public native/RHI handles, native GPU HUD/sprite overlay output, and general production renderer quality unsupported.
- GREEN 2026-05-01: strict Vulkan selected package validation initially crashed after scene/HUD submission. Validation-layer evidence first showed `vkGetPhysicalDeviceSurfaceSupportKHR` was queried with a queue-family index that did not belong to the same native instance/physical-device snapshot. `engine/rhi/vulkan/src/vulkan_backend.cpp` now refreshes surface-probe queue-family snapshots from the same native instance handles before probing support.
- GREEN 2026-05-01: after the Vulkan surface-probe fix, the remaining strict Vulkan crash was isolated to `renderer_.end_frame()` when HUD sprite submissions were replayed through the 3D scene/postprocess material pipeline. `RhiPostprocessFrameRenderer` and `RhiDirectionalShadowSmokeFrameRenderer` now count sprite submissions as diagnostics in this scene package proof instead of drawing them through scene pipelines; native GPU HUD or sprite overlay output stays a planned follow-up.
- GREEN 2026-05-01: strict Vulkan selected package validation passed outside the sandbox with `tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game -RequireVulkanShaders -SmokeArgs @('--smoke', '--require-config', 'runtime/sample_desktop_runtime_game.config', '--require-scene-package', 'runtime/sample_desktop_runtime_game.geindex', '--require-vulkan-scene-shaders', '--video-driver', 'windows', '--require-vulkan-renderer', '--require-scene-gpu-bindings', '--require-postprocess', '--require-postprocess-depth-input', '--require-directional-shadow', '--require-directional-shadow-filtering')`; installed smoke reported `renderer=vulkan`, `camera_primary=1`, `camera_controller_ticks=2`, `hud_boxes=2`, `scene_gpu_status=ready`, `postprocess_status=ready`, `postprocess_depth_input_ready=1`, `directional_shadow_status=ready`, `directional_shadow_ready=1`, `directional_shadow_filter_mode=fixed_pcf_3x3`, `directional_shadow_filter_taps=9`, `directional_shadow_filter_radius_texels=1`, and `framegraph_passes=3`.
- GREEN 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed.
- GREEN 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed.
- GREEN 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed.
- GREEN 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1` passed as diagnostic-only with D3D12 DXIL ready, Vulkan SPIR-V ready, DXC SPIR-V CodeGen ready, and Metal `metal`/`metallib` missing as the expected Apple-host-gated blocker.
- GREEN 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` passed with 14/14 CTest tests.
- GREEN 2026-05-01: selected D3D12 package validation first hit the known sandbox-only vcpkg 7zip `CreateFileW stdin failed with 5` blocker, then passed outside the sandbox with `tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game`; installed smoke reported `renderer=d3d12`, `camera_primary=1`, `camera_controller_ticks=2`, `hud_boxes=2`, `scene_gpu_status=ready`, `postprocess_depth_input_ready=1`, `directional_shadow_ready=1`, and `framegraph_passes=3`.
- GREEN 2026-05-01: created `docs/superpowers/plans/2026-05-01-native-gpu-ui-overlay-foundation-v1.md`, updated `docs/superpowers/plans/README.md`, and moved `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` to that next C-phase slice.
- GREEN 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` passed and emitted the updated production loop with `currentActivePlan=docs/superpowers/plans/2026-05-01-native-gpu-ui-overlay-foundation-v1.md`.
- GREEN 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with 28/28 CTest tests. Diagnostic-only blockers remained Metal tools missing, Apple packaging requiring macOS/Xcode, Android release signing/device smoke not fully configured, and tidy compile database availability.
