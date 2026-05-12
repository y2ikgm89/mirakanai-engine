# Package-Visible Directional Shadow Filtering v0 Implementation Plan (2026-04-30)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Promote the existing package-visible directional shadow smoke from single sampled-depth comparison to a fixed filtered receiver proof for `sample_desktop_runtime_game`.

**Architecture:** Keep shadow filtering as backend-neutral `mirakana_renderer` receiver policy plus host-owned SDL3 presentation wiring. D3D12/Vulkan package paths reuse the existing target-specific shadow receiver artifacts and report only first-party readiness/filter diagnostics; native handles, descriptors, swapchain frames, and `IRhiDevice` stay private.

**Tech Stack:** C++23, `mirakana_renderer`, `mirakana_runtime_host_sdl3_presentation`, `mirakana_rhi`/D3D12/Vulkan private backends, HLSL/DXIL/SPIR-V shader artifacts, CMake desktop runtime package metadata, PowerShell validation.

---

## Scope

- Add fixed PCF receiver policy to `ShadowReceiverDesc` / `ShadowReceiverPlan`.
- Use fixed 3x3 sampled-depth PCF in `sample_desktop_runtime_game` shadow receiver shader and Vulkan test receiver shader.
- Report package-visible filter mode/tap count/radius through first-party SDL3 presentation report fields and sample status output.
- Extend selected installed package validation to require filtered directional shadow evidence.
- Sync roadmap/gap/docs/manifest/Codex/Claude guidance and static AI checks.

## Non-Goals

- No cascaded shadows, atlases, stable light-space camera policy, contact shadows, PCSS, VSM, EVSM, Metal presentation, editor controls, hardware comparison samplers, GPU markers, or full production shadow authoring.

## Task 1: Receiver Policy Tests

**Files:**
- Modify: `tests/unit/renderer_rhi_tests.cpp`
- Modify: `engine/renderer/include/mirakana/renderer/shadow_map.hpp`
- Modify: `engine/renderer/src/shadow_map.cpp`

- [x] **Step 1: Write failing tests**

Add expectations that a valid receiver plan defaults to `fixed_pcf_3x3`, radius `1.0`, and `9` taps, and that invalid filter mode/radius emits deterministic diagnostics.

- [x] **Step 2: Verify RED**

Run: `cmake --build --preset dev --target mirakana_renderer_tests`

Expected: compile fails because the new `ShadowReceiverFilterMode` fields and diagnostics do not exist.

- [x] **Step 3: Implement minimal policy**

Add `ShadowReceiverFilterMode`, `invalid_filter_mode`, `invalid_filter_radius_texels`, plan fields, finite non-negative radius validation, and deterministic tap-count derivation.

- [x] **Step 4: Verify GREEN**

Run: `cmake --build --preset dev --target mirakana_renderer_tests`
Run: `ctest --test-dir out\build\dev -C Debug --output-on-failure -R mirakana_renderer_tests`

## Task 2: Filtered Shader Proof

**Files:**
- Modify: `tests/unit/d3d12_rhi_tests.cpp`
- Modify: `tests/shaders/vulkan_shadow_receiver.hlsl`
- Modify: `games/sample_desktop_runtime_game/shaders/runtime_scene.hlsl`

- [x] **Step 1: Write failing D3D12 readback test**

Add a D3D12 readback test that writes a half-screen shadow depth edge and expects a receiver boundary pixel to be neither fully lit nor fully shadowed under fixed 3x3 PCF.

- [x] **Step 2: Verify RED**

Run: `cmake --build --preset dev --target mirakana_d3d12_rhi_tests`
Run: `ctest --test-dir out\build\dev -C Debug --output-on-failure -R mirakana_d3d12_rhi_tests`

Expected: new PCF boundary expectation fails while the existing point-sampled receiver remains all-lit or all-shadowed.

- [x] **Step 3: Implement shader filtering**

Replace single `SampleLevel` receiver comparison with deterministic 3x3 sampled-depth PCF using `GetDimensions`, one-texel radius, receiver depth `0.5`, and the existing bias/intensity policy.

- [x] **Step 4: Verify GREEN**

Run the same D3D12 focused build and CTest.

## Task 3: Package-Visible Report Fields

**Files:**
- Modify: `engine/renderer/include/mirakana/renderer/rhi_directional_shadow_smoke_frame_renderer.hpp`
- Modify: `engine/renderer/src/rhi_directional_shadow_smoke_frame_renderer.cpp`
- Modify: `engine/runtime_host/sdl3/include/mirakana/runtime_host/sdl3/sdl_desktop_presentation.hpp`
- Modify: `engine/runtime_host/sdl3/src/sdl_desktop_presentation.cpp`
- Modify: `tests/unit/runtime_host_sdl3_public_api_compile.cpp`
- Modify: `tests/unit/runtime_host_sdl3_tests.cpp`
- Modify: `games/sample_desktop_runtime_game/main.cpp`
- Modify: `tools/validate-installed-desktop-runtime.ps1`
- Modify: `games/CMakeLists.txt`

- [x] **Step 1: Write failing public/report tests**

Require `directional_shadow_filter_mode=fixed_pcf_3x3`, `directional_shadow_filter_taps=9`, and `directional_shadow_filter_radius_texels=1` in first-party reports and package smoke validation when `--require-directional-shadow-filtering` is present.

- [x] **Step 2: Verify RED**

Run: `cmake --build --preset dev --target mirakana_runtime_host_sdl3_tests`

Expected: compile fails or tests fail because filter report fields and CLI validation do not exist.

- [x] **Step 3: Implement report plumbing**

Carry `ShadowReceiverPlan` filter metadata into `RhiDirectionalShadowSmokeFrameRenderer`, `SdlDesktopPresentationReport`, sample status lines, and installed smoke validation. `--require-directional-shadow-filtering` implies `--require-directional-shadow`.

- [x] **Step 4: Verify GREEN**

Run: `cmake --build --preset dev --target mirakana_runtime_host_sdl3_tests`
Run: `ctest --test-dir out\build\dev -C Debug --output-on-failure -R mirakana_runtime_host_sdl3_tests`

## Task 4: Docs, Manifest, Skills, Subagents

**Files:**
- Modify: `docs/roadmap.md`
- Modify: `docs/architecture.md`
- Modify: `docs/rhi.md`
- Modify: `docs/testing.md`
- Modify: `docs/workflows.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/rendering-change/SKILL.md`
- Modify: `.claude/skills/gameengine-rendering/SKILL.md`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`
- Modify: `.codex/agents/rendering-auditor.toml`
- Modify: `.claude/agents/rendering-auditor.md`
- Modify: `.codex/agents/gameplay-builder.toml`
- Modify: `.claude/agents/gameplay-builder.md`
- Modify: `games/sample_desktop_runtime_game/game.agent.json`
- Modify: `tools/check-ai-integration.ps1`

- [x] **Step 1: Update capability language**

Replace "no PCF/filtering" claims for the package-visible smoke with "fixed sampled-depth 3x3 PCF filtering is implemented" while keeping hardware comparison samplers, cascades, atlases, Metal, and production shadow authoring unclaimed.

- [x] **Step 2: Verify static checks**

Run: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`
Run: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`

## Task 5: Validation

- [x] **Focused renderer/runtime validation**

Run: `cmake --build --preset dev --target mirakana_renderer_tests mirakana_runtime_host_sdl3_tests mirakana_d3d12_rhi_tests`
Run: `ctest --test-dir out\build\dev -C Debug --output-on-failure -R "mirakana_renderer_tests|mirakana_runtime_host_sdl3_tests|mirakana_d3d12_rhi_tests"`

- [x] **Shader/package validation**

Run: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`
Run: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`
Run: `tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game`

- [x] **Repository validation**

Run: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`
Run: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`
Run: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`
Run: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`
Run: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1`
Run: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`

## Done When

- [x] `sample_desktop_runtime_game --require-directional-shadow-filtering` reports ready fixed 3x3 PCF metadata on ready D3D12/Vulkan package lanes.
- [x] D3D12 readback proves a filtered boundary pixel distinct from fully lit and fully shadowed receiver pixels.
- [x] Renderer/host public fields remain first-party and native-handle-free.
- [x] Docs, manifest, Codex skills/subagents, Claude skills/subagents, and AI checks are synchronized.
- [x] Focused validation and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass, or exact host-gated blockers are recorded.

## Validation Evidence

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`: PASS.
- Focused dev build/CTest for `mirakana_renderer_tests`, `mirakana_runtime_host_tests`, and `mirakana_d3d12_rhi_tests`: PASS.
- `mirakana_renderer_tests` post-review RED/GREEN added coverage for incoherent `RhiDirectionalShadowSmokeFrameRendererDesc` filter metadata.
- Desktop-runtime focused build/CTest for `mirakana_runtime_host_sdl3_tests`, `mirakana_runtime_host_sdl3_public_api_compile`, and `sample_desktop_runtime_game_smoke`: PASS after sandbox-gated vcpkg 7zip extraction was rerun with elevated approval.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`: D3D12 DXIL, Vulkan SPIR-V, and DXC SPIR-V CodeGen ready; Metal `metal`/`metallib` missing diagnostic-only.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`: PASS, 12/12 tests after sandbox-gated vcpkg 7zip extraction was rerun with elevated approval.
- `tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game`: PASS with installed D3D12 status fields `directional_shadow_filter_mode=fixed_pcf_3x3`, `directional_shadow_filter_taps=9`, `directional_shadow_filter_radius_texels=1`.
- Strict Vulkan selected package validation with `-RequireVulkanShaders` and `--require-directional-shadow-filtering`: PASS on the ready Windows/Vulkan host with SPIR-V artifacts validated.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1`: config PASS; Visual Studio dev preset `compile_commands.json` missing, diagnostic-only.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS, 22/22 tests. Known diagnostic-only host gates remain Metal/Apple tooling, Android signing/device smoke, and strict clang-tidy compile database.
