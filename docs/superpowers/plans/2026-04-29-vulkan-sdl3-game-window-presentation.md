# Vulkan SDL3 Game-Window Presentation Implementation Plan (2026-04-29)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a minimal host-owned SDL3/Vulkan game-window presentation path that can create a private Vulkan `IRhiDevice`, create a swapchain-backed `RhiFrameRenderer` from precompiled SPIR-V artifacts, and fall back deterministically to `NullRenderer` without exposing SDL, Win32, Vulkan, or `IRhiDevice` handles to gameplay.

**Architecture:** Extend `mirakana_runtime_host_sdl3_presentation` as the adapter boundary. Keep SDL3 window property probing, Vulkan runtime probing, Vulkan device/swapchain ownership, and renderer creation inside the host adapter. Reuse the existing `mirakana_rhi_vulkan` backend-neutral `IRhiDevice` bridge and `RhiFrameRenderer`. Do not add runtime shader compilation; Vulkan shaders are build/package artifacts.

**Tech Stack:** C++23, `mirakana_runtime_host_sdl3_presentation`, `mirakana_rhi_vulkan`, `mirakana_renderer::RhiFrameRenderer`, SDL3, focused SDL3 runtime-host tests, shader toolchain validation, desktop-runtime validation.

---

## Context

- D3D12 SDL3 game-window creation and packaged scene GPU binding are implemented.
- `mirakana_rhi_vulkan` already owns private runtime instance/device/surface/swapchain and exposes a backend-neutral `IRhiDevice` bridge.
- The Windows host reports Vulkan SPIR-V and DXC SPIR-V CodeGen readiness through `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`; Metal remains Apple-host gated.
- `SdlDesktopPresentationBackend` currently distinguishes `null_renderer` and `d3d12` only.

## Constraints

- Do not expose `VkInstance`, `VkDevice`, `VkSurfaceKHR`, SDL window pointers, Win32 `HWND`, or `IRhiDevice` getters through public game APIs.
- Keep Vulkan presentation optional and host-gated; if the Vulkan runtime, driver, surface support, or SPIR-V artifacts are unavailable, report deterministic fallback diagnostics.
- Preserve existing D3D12 behavior and default desktop runtime/package lanes.
- Do not add third-party dependencies.
- Do not mix scene GPU binding parity, PBR, GPU markers, or material editor work into this slice.

## Done When

- [x] Public SDL3 presentation types can request Vulkan renderer creation with first-party shader bytecode descriptors and report `vulkan` backend status without native handles.
- [x] Dummy/null fallback tests cover invalid or unavailable Vulkan requests with deterministic diagnostics.
- [x] A real Windows SDL3/Vulkan strict smoke path exists when the host Vulkan runtime and SPIR-V artifacts are available.
- [x] Build/package metadata can represent target-specific Vulkan SPIR-V artifacts without breaking existing D3D12 package validation.
- [x] Docs, roadmap, gap analysis, manifest, rendering skills, and Codex/Claude rendering-auditor guidance distinguish implemented D3D12, implemented-or-host-gated Vulkan, and Apple-gated Metal honestly.
- [x] Focused tests, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`, relevant package validation, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or exact Vulkan runtime/toolchain blockers are recorded.

---

### Task 1: RED Vulkan Request And Fallback Tests

**Files:**
- Modify: `tests/unit/runtime_host_sdl3_tests.cpp`
- Modify: `engine/runtime_host/sdl3/include/mirakana/runtime_host/sdl3/sdl_desktop_presentation.hpp`
- Modify: `engine/runtime_host/sdl3/include/mirakana/runtime_host/sdl3/sdl_desktop_game_host.hpp`

- [x] Add tests proving dummy SDL presentation with a Vulkan request falls back to `NullRenderer` and reports `native_surface_unavailable` or `runtime_pipeline_unavailable`.
- [x] Add tests proving required Vulkan renderer creation rejects missing SPIR-V bytecode without falling back.
- [x] Add public API compile coverage if new request/status types are introduced.

### Task 2: Implement Private Vulkan Renderer Creation

**Files:**
- Modify: `engine/runtime_host/sdl3/src/sdl_desktop_presentation.cpp`
- Modify: `engine/runtime_host/sdl3/src/sdl_desktop_game_host.cpp`
- Modify: `engine/runtime_host/sdl3/CMakeLists.txt`

- [x] Add a backend preference/request path that preserves D3D12 default behavior and allows explicit Vulkan.
- [x] Probe SDL3 Win32 surface availability privately and map it into a first-party `rhi::SurfaceHandle`.
- [x] Use `mirakana_rhi_vulkan` runtime probes, device creation, swapchain creation, shader module creation, graphics pipeline creation, and `RhiFrameRenderer` without exposing native handles.
- [x] Return deterministic diagnostics for unavailable loader/runtime, unsupported surface/present queue, invalid SPIR-V, and renderer creation failures.

### Task 3: Shader Artifacts And Smoke Path

**Files:**
- Modify: `games/sample_desktop_runtime_shell/main.cpp`
- Modify: `games/sample_desktop_runtime_shell/shaders/runtime_shell.hlsl`
- Modify: `games/CMakeLists.txt`
- Modify: `tools/validate-installed-desktop-runtime.ps1`

- [x] Build Vulkan SPIR-V artifacts for the selected smoke sample when DXC SPIR-V CodeGen is available.
- [x] Add `--require-vulkan-shaders` and `--require-vulkan-renderer` strict smoke flags.
- [x] Keep default dummy/Null and D3D12 lanes intact.

### Task 4: Documentation And Agent Sync

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/architecture.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/rendering-change/SKILL.md`
- Modify: `.claude/skills/gameengine-rendering/SKILL.md`
- Modify: `.codex/agents/rendering-auditor.toml`
- Modify: `.claude/agents/rendering-auditor.md`

- [x] Mark Vulkan SDL3 presentation as implemented only after a focused smoke passes on this host or as host-gated if runtime/driver validation cannot complete.
- [x] Keep Metal presentation explicitly Apple-host gated.

### Task 5: Verification

- [x] Run focused SDL3 runtime-host tests.
- [x] Run strict Vulkan smoke when host runtime and SPIR-V artifacts are available.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`.
- [x] Run relevant package validation.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record diagnostic-only blockers explicitly.

## Validation Evidence

- Focused build: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; & $tools.CMake --build --preset desktop-runtime --target mirakana_runtime_host_sdl3_tests mirakana_runtime_host_sdl3_public_api_compile sample_desktop_runtime_shell mirakana_backend_scaffold_tests` passed after validating the public Vulkan request types and adding private Vulkan backend access needed by `mirakana_rhi_vulkan`.
- Focused CTest: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; & $tools.CTest --preset desktop-runtime --output-on-failure -R "mirakana_runtime_host_sdl3_tests|mirakana_runtime_host_sdl3_public_api_compile|mirakana_backend_scaffold_tests|sample_desktop_runtime_shell(_vulkan_shader_artifacts)?_smoke"` passed, 5/5 tests.
- Strict source-tree Vulkan smoke: `out\build\desktop-runtime\games\Debug\sample_desktop_runtime_shell.exe --smoke --video-driver windows --require-vulkan-shaders --require-vulkan-renderer` passed with `renderer=vulkan`, `frames=2`, and `game_frames=2`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`: diagnostic-only PASS; D3D12 DXIL and Vulkan SPIR-V are ready through `C:\VulkanSDK\1.4.341.1\Bin\dxc.exe` and `spirv-val.exe`, while Metal `metal` and `metallib` are missing on this Windows host.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`: PASS after the known sandbox vcpkg 7zip `CreateFileW stdin failed with 5` blocker was rerun with approved escalation; 9/9 tests passed including D3D12 and Vulkan shell shader artifact smokes.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1`: PASS after the same sandbox vcpkg escalation; default installed shell validation checked D3D12 DXIL and Vulkan SPIR-V artifacts and created the desktop runtime CPack ZIP.
- Installed strict Vulkan smoke: `out\install\desktop-runtime-release\bin\sample_desktop_runtime_shell.exe --smoke --video-driver windows --require-vulkan-shaders --require-vulkan-renderer` passed with `renderer=vulkan`, `frames=2`, and `game_frames=2`.
- Selected package regression: `tools\package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game` PASS after the same sandbox vcpkg escalation; installed smoke reported `renderer=d3d12`, `scene_gpu_status=ready`, `scene_gpu_mesh_bindings=1`, `scene_gpu_material_bindings=1`, `scene_gpu_mesh_resolved=2`, and `scene_gpu_material_resolved=2`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS; diagnostic-only blockers remain Metal shader/library tools missing, Apple packaging missing macOS/Xcode tools, Android release signing not configured, Android device smoke not connected, and strict tidy compile database unavailable for the active generator.
