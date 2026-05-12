# Production Renderer Lit Material v0 Implementation Plan (2026-04-29)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Promote the runtime scene path from textured/unlit proof toward a minimal production lit material path with position+normal+uv mesh payloads, base-color texture/material factors, and one directional light.

**Architecture:** Extend first-party cooked mesh/runtime upload contracts to support an interleaved position+normal+uv vertex layout while preserving the existing position-only path. Keep `mirakana_runtime` and `mirakana_scene` RHI-free; `mirakana_runtime_rhi`, `mirakana_runtime_scene_rhi`, renderer/RHI, and SDL3 host adapters own GPU binding and shader validation. Update the sample scene shader to a minimal Lambert path using existing material descriptors and one host-owned direct-light constant path without exposing native handles.

**Tech Stack:** C++23, `mirakana_assets` mesh/material source documents, `mirakana_runtime`, `mirakana_runtime_rhi`, `mirakana_runtime_scene_rhi`, `mirakana_renderer`, `mirakana_scene_renderer`, D3D12/Vulkan RHI smoke paths, `sample_desktop_runtime_game`, DXC DXIL/SPIR-V artifact lanes.

---

## Context

- Runtime cooked scene packages can now reach D3D12 and Vulkan visible game-window proofs with retained scene GPU bindings.
- `MaterialDefinition` already carries `base_color`, `emissive`, `metallic`, `roughness`, surface mode, and texture-slot metadata.
- `SceneRenderPacket` already carries light components, and `mirakana_scene_renderer` can turn them into backend-neutral `SceneLightCommand` values.
- Runtime mesh upload currently defaults to fixed float32x3 position stride; the sample cooked mesh has no normals or UVs.
- `sample_desktop_runtime_game` currently samples a base-color texture at a constant UV and does not use mesh normals or scene lights.

## Constraints

- Keep `mirakana_scene` independent from renderer/RHI/native APIs.
- Keep native D3D12/Vulkan/Metal handles private to backend modules.
- Do not add new third-party dependencies.
- Do not add full PBR BRDF, normal maps, multiple lights, shadows, post-processing, render graph, editor UI, Metal presentation, or GPU markers in this slice.
- Keep `NullRenderer` and headless samples deterministic.
- Public API additions remain first-party `mirakana::` value types only and require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.

## Done When

- [x] Runtime mesh upload can derive position-only and position+normal+uv vertex layout contracts from cooked mesh payload metadata.
- [x] `mirakana_runtime_scene_rhi` uploads the lit mesh layout automatically for cooked meshes with normals and UVs while preserving existing position-only tests.
- [x] `sample_desktop_runtime_game` ships a cooked triangle mesh with normals/UVs and D3D12/Vulkan vertex input descriptors for position+normal+uv.
- [x] The sample scene shader uses base-color texture, material factors, emissive, and one deterministic directional light for a minimal Lambert lit path.
- [x] D3D12 and Vulkan package/source smokes continue to report `scene_gpu_status=ready` and scene GPU resolved counters on this Windows host.
- [x] Docs, roadmap, gap analysis, manifest, skills, and Codex/Claude guidance describe this as Lit Material v0, not completed PBR/shadow/postprocess production rendering.
- [x] Focused runtime-rhi/runtime-scene-rhi/runtime-host tests, shader toolchain diagnostics, desktop runtime validation, selected package validation, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or exact host blockers are recorded.

---

### Task 1: RED Runtime Mesh Layout Tests

**Files:**
- Modify: `tests/unit/runtime_rhi_tests.cpp`
- Modify: `tests/unit/runtime_scene_rhi_tests.cpp`

- [x] Add tests that a position-only cooked mesh still uploads with stride 12 and position-only vertex attributes.
- [x] Add tests that a cooked mesh with `has_normals=true` and `has_uvs=true` uploads with stride 32 and position/normal/uv vertex attributes.
- [x] Add tests that unsupported partial layouts such as normals-without-uvs or uvs-without-normals are rejected before buffers are created.
- [x] Add `mirakana_runtime_scene_rhi` tests proving lit mesh payloads produce `MeshGpuBinding::vertex_stride == 32`.

### Task 2: Runtime Mesh Layout Implementation

**Files:**
- Modify: `engine/runtime_rhi/include/mirakana/runtime_rhi/runtime_upload.hpp`
- Modify: `engine/runtime_rhi/src/runtime_upload.cpp`
- Modify: `engine/runtime_scene_rhi/src/runtime_scene_rhi.cpp`

- [x] Add first-party runtime mesh layout constants for position-only stride 12 and lit position+normal+uv stride 32.
- [x] Add a helper that derives `RuntimeMeshUploadOptions` from `RuntimeMeshPayload` metadata.
- [x] Keep manual upload options available for tests/backends that explicitly need a custom layout while still rejecting partial normal/UV cooked metadata before buffer creation.
- [x] Have `mirakana_runtime_scene_rhi` use derived mesh upload options per cooked mesh unless a caller explicitly overrides mesh upload options.

### Task 3: Sample Lit Mesh And Shader Path

**Files:**
- Modify: `games/sample_desktop_runtime_game/runtime/assets/desktop-runtime/triangle.mesh`
- Modify: `games/sample_desktop_runtime_game/shaders/runtime_scene.hlsl`
- Modify: `games/sample_desktop_runtime_game/main.cpp`

- [x] Replace the sample cooked mesh vertex bytes with interleaved position, normal, and UV values.
- [x] Update D3D12 and Vulkan scene renderer descriptors to use stride 32 and position/normal/uv vertex attributes.
- [x] Update the HLSL shader so VS forwards normal/uv and PS evaluates a deterministic one-directional-light Lambert term using the base-color texture, material factors, and emissive factors.
- [x] Preserve the existing material descriptor register contract so package metadata and host-owned scene GPU binding remain unchanged.

### Task 3a: Reviewer Follow-Up Hardening

**Files:**
- Modify: `engine/tools/src/asset_import_adapters.cpp`
- Modify: `engine/runtime_rhi/src/runtime_upload.cpp`
- Modify: `engine/runtime_host/sdl3/src/sdl_desktop_presentation.cpp`
- Modify: `tests/unit/tools_tests.cpp`
- Modify: `tests/unit/runtime_rhi_tests.cpp`
- Modify: `tests/unit/runtime_host_sdl3_tests.cpp`

- [x] Cook glTF POSITION/NORMAL/TEXCOORD_0 triangle primitives as interleaved position+normal+uv bytes instead of metadata-only normal/UV flags.
- [x] Reject partial glTF normal/UV primitives before cooked mesh artifacts are written.
- [x] Reject partial normal/UV runtime mesh payloads even when callers disable automatic layout derivation.
- [x] Validate SDL3 D3D12/Vulkan scene renderer request vertex input against referenced cooked mesh payload layouts before reporting `scene_gpu_status=ready`.

### Task 4: Documentation And Agent Sync

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/architecture.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/rhi.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/rendering-change/SKILL.md`
- Modify: `.claude/skills/gameengine-rendering/SKILL.md`
- Modify: `.codex/agents/rendering-auditor.toml`
- Modify: `.claude/agents/rendering-auditor.md`

- [x] Record the capability as Lit Material v0 with position+normal+uv mesh payloads and one directional light in the sample RHI-backed scene path.
- [x] Keep PBR BRDF shader implementation, normal maps, multiple lights, shadows, render graph, post-processing, and GPU marker support listed as follow-up work.
- [x] Keep D3D12/Vulkan/Metal backend readiness claims unchanged unless native shader/readback proofs are added in a later slice.

### Task 5: Verification

- [x] Run focused runtime-rhi, runtime-scene-rhi, renderer, scene-renderer, and runtime-host tests through the active CMake preset.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`.
- [x] Run selected package validation for `sample_desktop_runtime_game`.
- [x] Run strict Vulkan selected package validation when Vulkan runtime/toolchain is ready.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record any diagnostic-only host/toolchain blockers explicitly.

## Implementation Evidence

- `mirakana_runtime_rhi` now derives `RuntimeMeshVertexLayoutDesc` and upload options from cooked mesh payload metadata. Position-only meshes keep stride 12; meshes with normals and UVs use interleaved position+normal+uv stride 32; partial normals/UV layouts are rejected before buffer creation.
- `mirakana_runtime_scene_rhi` consumes those derived mesh upload options through the existing package/packet palette bridge, so lit cooked meshes retain `MeshGpuBinding::vertex_stride == 32` without adding RHI dependencies to `mirakana_runtime` or `mirakana_scene`.
- `sample_desktop_runtime_game` now ships a lit triangle mesh with normals/UVs, a lit material marker, a deterministic key light in the scene payload, and D3D12/Vulkan scene renderer descriptors that use position, normal, and texcoord vertex attributes.
- `games/sample_desktop_runtime_game/shaders/runtime_scene.hlsl` preserves the existing material descriptor register contract while sampling the base-color texture at mesh UVs and applying material base color, emissive factors, and a deterministic one-directional-light Lambert term.
- Reviewer hardening closed three layout drift paths: optional glTF import now emits actual interleaved POSITION/NORMAL/TEXCOORD_0 bytes for lit triangle primitives and rejects partial lit attributes; runtime mesh upload rejects partial normal/UV metadata even when automatic derivation is disabled; SDL3 scene presentation rejects scene renderer requests whose supplied vertex input does not match the referenced cooked mesh layout before native surface creation or ready status reporting.
- Focused validation run before final full validation: `cmake --build --preset dev --target mirakana_runtime_rhi_tests mirakana_runtime_scene_rhi_tests` PASS; `ctest --preset dev --output-on-failure -R "mirakana_runtime_rhi_tests|mirakana_runtime_scene_rhi_tests"` PASS; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1` PASS as diagnostic-only with D3D12 DXIL, Vulkan SPIR-V, and DXC SPIR-V CodeGen ready while Metal `metal`/`metallib` remain missing; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` PASS after sandbox-blocked vcpkg 7zip extraction was rerun with elevated permissions; `tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game` PASS with installed `renderer=d3d12` and `scene_gpu_status=ready`; strict `-RequireVulkanShaders` package PASS with installed `renderer=vulkan`, `presentation_selected=vulkan`, and `scene_gpu_status=ready`.
- Reviewer follow-up validation: `cmake --build --preset dev --target mirakana_runtime_rhi_tests mirakana_tools_tests` PASS; `ctest --preset dev --output-on-failure -R "mirakana_runtime_rhi_tests|mirakana_tools_tests"` PASS; `cmake --build --preset desktop-runtime --target mirakana_runtime_host_sdl3_tests` PASS after the known vcpkg 7zip sandbox failure was rerun with elevated permissions; `ctest --preset desktop-runtime --output-on-failure -R "mirakana_runtime_host_sdl3_tests"` PASS; `cmake --build --preset asset-importers --target mirakana_tools_tests` PASS after the same sandbox gate was rerun with elevated permissions; `ctest --preset asset-importers --output-on-failure -R "mirakana_tools_tests"` PASS.
- Final slice validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` PASS after the known sandbox-blocked vcpkg 7zip extraction was rerun with elevated permissions; `tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game` PASS with installed `renderer=d3d12` and `scene_gpu_status=ready`; strict `-RequireVulkanShaders` selected package validation PASS with installed `renderer=vulkan`, `presentation_selected=vulkan`, and `scene_gpu_status=ready`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` PASS with 19/19 CTest tests passing. Host-gated diagnostics remain unchanged: Metal `metal`/`metallib` missing, Apple packaging requires macOS/Xcode, Android release signing not configured, Android device smoke not connected, and strict tidy remains diagnostic-only when the Visual Studio generator compile database is unavailable at tidy-check start.
