# Native RHI Desktop Game Window Presentation Implementation Plan (2026-04-28)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Promote the editor-independent desktop game runtime shell from a windowed `NullRenderer` sample toward native RHI-backed game-window presentation without exposing native handles to gameplay code.

**Architecture:** Keep `mirakana::GameApp`, `mirakana::DesktopGameRunner`, and gameplay code on public `mirakana::` contracts. First harden the backend-neutral swapchain path by making `RhiFrameRenderer` resize its owned swapchain when the desktop host window changes size, then add host-only presentation helpers and optional D3D12/SDL3 integration in later tasks. Native OS, SDL, D3D12, Vulkan, and Metal handles remain inside platform/RHI/runtime-host adapter modules.

**Tech Stack:** C++23, CMake target-based modules, `mirakana_runtime_host`, `mirakana_renderer`, `mirakana_rhi`, optional `mirakana_platform_sdl3`, optional `mirakana_rhi_d3d12`, first-party test harness, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

---

## File Structure

- Modify `tests/unit/renderer_rhi_tests.cpp` for swapchain-backed `RhiFrameRenderer` resize coverage.
- Modify `tests/unit/runtime_host_tests.cpp` for `DesktopGameRunner` driving an RHI swapchain renderer resize through public `IWindow`/`IRenderer` contracts.
- Modify `engine/renderer/src/rhi_frame_renderer.cpp` so `RhiFrameRenderer::resize` calls `IRhiDevice::resize_swapchain` when constructed with a swapchain.
- Modify `docs/superpowers/plans/README.md` to register this active slice.
- Modify `docs/roadmap.md`, `docs/architecture.md`, `docs/rhi.md`, `docs/ai-game-development.md`, `docs/specs/2026-04-27-engine-essential-gap-analysis.md`, `engine/agent/manifest.json`, and game-development skills as capability changes land.

## Task 1: Swapchain-Backed Renderer Resize Contract

**Files:**
- Modify: `tests/unit/renderer_rhi_tests.cpp`
- Modify: `engine/renderer/src/rhi_frame_renderer.cpp`

- [x] **Step 1: Write the failing renderer test**

Add a test that constructs `mirakana::RhiFrameRenderer` with `mirakana::rhi::NullRhiDevice`, a swapchain, and a valid graphics pipeline, calls `renderer.resize({128, 72})`, then asserts:

- `renderer.backbuffer_extent()` is `128x72`
- `device.stats().swapchain_resizes == 1`

- [x] **Step 2: Run the renderer test to verify it fails**

Run:

```powershell
. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --preset dev; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_renderer_tests; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R mirakana_renderer_tests
```

Expected: the new test fails because `RhiFrameRenderer::resize` updates only its local extent and does not call `IRhiDevice::resize_swapchain`.

- [x] **Step 3: Implement swapchain resize propagation**

Update `RhiFrameRenderer::resize` so a non-zero `swapchain_` calls:

```cpp
device_->resize_swapchain(swapchain_, rhi::Extent2D{extent.width, extent.height});
```

before storing the new renderer extent. Keep texture-target renderers unchanged.

- [x] **Step 4: Run the renderer test to verify it passes**

Review hardening added in this step: `RhiFrameRenderer` now rejects resize during an active frame, and begin-frame failure cleanup keeps the acquired command list alive long enough to explicitly release any acquired swapchain frame before rethrowing.

Run the same `mirakana_renderer_tests` command. Expected: all renderer tests pass.

## Task 2: Desktop Runner Drives RHI Swapchain Resize

**Files:**
- Modify: `tests/unit/runtime_host_tests.cpp`

- [x] **Step 1: Write the failing desktop-host integration test**

Add a test that creates:

- `mirakana::HeadlessWindow` at `800x450`
- `mirakana::rhi::NullRhiDevice`
- a `mirakana::rhi::SwapchainHandle` initially `320x180`
- a valid `mirakana::RhiFrameRenderer` backed by that swapchain
- `mirakana::DesktopGameRunner` with `resize_renderer_to_window = true`

The app should stop after one update and assert the renderer extent is `800x450`. The test should also assert the RHI resize call received `800x450` and `swapchain_resizes == 1` through a recording `IRhiDevice` wrapper, without widening `NullRhiDevice` public API just for test inspection.

- [x] **Step 2: Run the runtime-host test to verify it passes after Task 1**

Run:

```powershell
. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_runtime_host_tests; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R mirakana_runtime_host_tests
```

Expected: this passes after Task 1. If it fails, fix the host/renderer boundary rather than adding game-facing native handle access.

## Task 3: Documentation And Manifest Sync

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/architecture.md`
- Modify: `docs/rhi.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`

- [x] **Step 1: Record the active slice**

Register this plan as the active slice in the plan registry and state that Task 1 is the backend-neutral foundation for native desktop game-window presentation.

- [x] **Step 2: Update capability guidance**

Document that `RhiFrameRenderer` swapchain-backed resize now participates in `DesktopGameRunner` window resizing. Keep D3D12/Vulkan/Metal native window surface extraction and shader artifact readiness as follow-up tasks until adapter-specific tests exist.

## Task 4: Verification

**Files:**
- No direct file edits.

- [x] **Step 1: Run targeted validation**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1
```

Expected: public API boundaries and JSON contracts pass; optional GUI lane still builds `sample_desktop_runtime_shell`.

- [x] **Step 2: Run default validation**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Expected: default build and CTest pass. Report any diagnostic-only host/tool blockers exactly.
