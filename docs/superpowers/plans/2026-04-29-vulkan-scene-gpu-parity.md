# Vulkan Scene GPU Parity Implementation Plan (2026-04-29)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Bring the installed cooked-scene desktop runtime proof closer to backend parity by adding a Vulkan scene GPU binding path alongside the existing D3D12 path, using precompiled SPIR-V scene shaders and private host-owned Vulkan RHI resources while keeping gameplay APIs native-handle-free.

**Architecture:** Extend `MK_runtime_host_sdl3_presentation` with a Vulkan scene renderer request mirroring the D3D12 scene renderer contract. Keep SDL3 surface probing, Vulkan runtime device creation, SPIR-V validation, swapchain/pipeline ownership, scene GPU palette construction, and renderer wrapping inside the host adapter. Reuse `MK_runtime_scene_rhi` and `SceneGpuBindingInjectingRenderer`; do not expose `IRhiDevice`, Vulkan handles, swapchain frames, or scene GPU palettes to gameplay.

**Tech Stack:** C++23, `MK_runtime_host_sdl3_presentation`, `MK_rhi_vulkan`, `MK_runtime_scene_rhi`, `sample_desktop_runtime_game`, build-output/installed SPIR-V artifacts, SDL3 Windows strict smoke validation.

---

## Context

- D3D12 packaged scene GPU injection is implemented and validated through selected package smoke with `scene_gpu_status=ready`.
- Vulkan SDL3 game-window presentation is implemented for the shell sample with precompiled SPIR-V and strict Windows smoke.
- Presentation reports now provide stable requested/selected backend and scene GPU status fields for package validation and tooling.
- This slice closes the prior gap where `sample_desktop_runtime_game` could prove scene GPU bindings only on D3D12; Vulkan scene GPU proof now remains host/runtime/toolchain-gated like the Vulkan presentation backend itself.

## Constraints

- Do not expose SDL, Win32, Vulkan, swapchain, RHI-device, shader-module, or scene GPU palette handles in game public APIs.
- Do not add runtime shader compilation or third-party dependencies.
- Preserve D3D12 package behavior and dummy `NullRenderer` fallback behavior.
- Keep this slice to Vulkan scene GPU parity; do not add PBR, shadows, GPU markers, editor Vulkan preview, or Metal presentation.
- Vulkan success remains Windows/runtime/toolchain-gated; default dummy validation must remain deterministic.

## Done When

- [x] Public SDL3 presentation/game-host descriptors can request a Vulkan scene renderer through first-party shader bytecode/package/packet metadata without native handles.
- [x] Unit tests cover missing/invalid Vulkan scene renderer requests, dummy-surface fallback, report rows, and game-host forwarding.
- [x] `sample_desktop_runtime_game` can load target-specific scene SPIR-V artifacts and request Vulkan scene GPU bindings when `--require-vulkan-renderer` / `--require-vulkan-scene-shaders` / `--require-scene-gpu-bindings` are selected.
- [x] CMake metadata, installed validation, and package lanes can declare and install target-specific Vulkan scene SPIR-V artifacts for the selected game target.
- [x] A ready Windows host can run strict source-tree and installed Vulkan scene GPU smokes with `presentation_selected=vulkan` and `scene_gpu_status=ready`.
- [x] Docs, roadmap, gap analysis, manifest, skills, and Codex/Claude guidance distinguish Vulkan scene GPU parity from Metal/Apple-host-gated work.
- [x] Focused tests, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`, relevant package validation, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or exact host blockers are recorded.

---

### Task 1: RED Vulkan Scene Request Tests

**Files:**
- Modify: `tests/unit/runtime_host_sdl3_tests.cpp`
- Modify: `tests/unit/runtime_host_sdl3_public_api_compile.cpp`

- [x] Add public compile coverage for a Vulkan scene renderer descriptor.
- [x] Add dummy fallback tests for missing scene SPIR-V/package/packet/input metadata.
- [x] Add fallback report checks for Vulkan scene renderer requests that cannot acquire a native surface.

### Task 2: Host-Owned Vulkan Scene Renderer

**Files:**
- Modify: `engine/runtime_host/sdl3/include/mirakana/runtime_host/sdl3/sdl_desktop_presentation.hpp`
- Modify: `engine/runtime_host/sdl3/include/mirakana/runtime_host/sdl3/sdl_desktop_game_host.hpp`
- Modify: `engine/runtime_host/sdl3/src/sdl_desktop_presentation.cpp`
- Modify: `engine/runtime_host/sdl3/src/sdl_desktop_game_host.cpp`

- [x] Add `SdlDesktopPresentationVulkanSceneRendererDesc` with SPIR-V bytecode, package, packet, topology, and vertex input metadata.
- [x] Create a private Vulkan scene renderer path that validates SPIR-V, builds `SceneGpuBindingPalette` through `MK_runtime_scene_rhi`, creates a pipeline from the palette material layout, wraps `RhiFrameRenderer` with `SceneGpuBindingInjectingRenderer`, and records backend report rows.
- [x] Preserve D3D12/default behavior and deterministic `NullRenderer` fallback diagnostics.

### Task 3: Sample, Build Metadata, And Package Validation

**Files:**
- Modify: `games/sample_desktop_runtime_game/main.cpp`
- Modify: `games/CMakeLists.txt`
- Modify: `CMakeLists.txt`
- Modify: `tools/validate-installed-desktop-runtime.ps1`
- Modify: `tools/package-desktop-runtime.ps1`
- Modify: `tools/validate-desktop-game-runtime.ps1`
- Modify: `games/sample_desktop_runtime_game/game.agent.json`
- Modify: `games/sample_desktop_runtime_game/README.md`

- [x] Compile target-specific scene SPIR-V artifacts when DXC SPIR-V CodeGen is available.
- [x] Add strict sample flags for requiring Vulkan scene shaders and Vulkan renderer selection.
- [x] Extend generated metadata/installed validation to cover target-specific Vulkan scene shader artifacts.
- [x] Ensure package smoke can validate presentation report fields and scene GPU counters for Vulkan on a ready host.

### Task 4: Documentation And Agent Sync

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/architecture.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/rhi.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.agents/skills/rendering-change/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-rendering/SKILL.md`
- Modify: `.codex/agents/gameplay-builder.toml`
- Modify: `.codex/agents/rendering-auditor.toml`
- Modify: `.claude/agents/gameplay-builder.md`
- Modify: `.claude/agents/rendering-auditor.md`

- [x] Mark Vulkan scene GPU parity implemented only after strict Windows smoke passes.
- [x] Keep Metal presentation and Apple validation explicitly host-gated.

### Task 5: Verification

- [x] Run focused SDL3 runtime-host build/tests.
- [x] Run strict source-tree Vulkan scene GPU smoke when host runtime and SPIR-V artifacts are available.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`.
- [x] Run default and selected package validation.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record diagnostic-only blockers explicitly.

## Validation Evidence

- `cmake --preset desktop-runtime`: sandbox run hit known `vcpkg` 7zip `CreateFileW stdin failed with 5`; the same command passed when rerun with elevated permissions.
- Focused desktop-runtime build passed for `MK_renderer_tests`, `MK_runtime_host_sdl3_tests`, `MK_runtime_host_sdl3_public_api_compile`, `MK_backend_scaffold_tests`, `sample_desktop_runtime_game`, and `sample_desktop_runtime_game_vulkan_shader_validation`.
- Focused `ctest --preset desktop-runtime --output-on-failure -R "MK_renderer_tests|MK_runtime_host_sdl3_tests|MK_runtime_host_sdl3_public_api_compile|MK_backend_scaffold_tests"` passed with `MK_VULKAN_TEST_RUNTIME_SCENE_VERTEX_SPV` and `MK_VULKAN_TEST_RUNTIME_SCENE_FRAGMENT_SPV` pointed at generated scene SPIR-V artifacts.
- Strict source-tree Vulkan scene smoke passed: `sample_desktop_runtime_game.exe --smoke --require-config runtime/sample_desktop_runtime_game.config --require-scene-package runtime/sample_desktop_runtime_game.geindex --require-vulkan-scene-shaders --video-driver windows --require-vulkan-renderer --require-scene-gpu-bindings` reported `presentation_selected=vulkan` and `scene_gpu_status=ready`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: PASS after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`: D3D12 DXIL ready, Vulkan SPIR-V ready, DXC SPIR-V CodeGen ready; Metal `metal` and `metallib` remain diagnostic-only host/toolchain blockers.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`: sandbox run hit known `vcpkg` 7zip `CreateFileW stdin failed with 5`; elevated rerun passed 9/9 tests.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1`: sandbox run hit known `vcpkg` 7zip `CreateFileW stdin failed with 5`; elevated rerun passed default `sample_desktop_runtime_shell` package validation and CPack ZIP creation.
- `tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game`: elevated rerun passed selected D3D12 package validation with `presentation_selected=d3d12` and `scene_gpu_status=ready`.
- `tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game -RequireVulkanShaders -SmokeArgs @('--smoke','--require-config','runtime/sample_desktop_runtime_game.config','--require-scene-package','runtime/sample_desktop_runtime_game.geindex','--require-vulkan-scene-shaders','--video-driver','windows','--require-vulkan-renderer','--require-scene-gpu-bindings')`: elevated rerun passed selected Vulkan package validation with `presentation_selected=vulkan` and `scene_gpu_status=ready`.
- Package script now passes `MK_REQUIRE_DESKTOP_RUNTIME_DXIL` and `MK_REQUIRE_DESKTOP_RUNTIME_SPIRV` as explicit `ON`/`OFF` values on every configure so a previous strict Vulkan package run cannot leak cached SPIR-V requirements into the default selected D3D12 package lane.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: PASS after registry, manifest, and next-plan sync.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS. Diagnostic-only blockers remained Metal `metal`/`metallib` missing, Apple packaging requiring macOS/Xcode tools, Android release signing not configured, Android device smoke not connected, and strict clang-tidy compile database unavailable for the active Visual Studio generator.
