# SDL3 Desktop Game Presentation Adapter Implementation Plan (2026-04-28)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add the first host-owned desktop game presentation adapter so `games/<name>` can request a windowed renderer through `mirakana_runtime_host_sdl3` without depending on editor, Dear ImGui, SDL3 event APIs, HWND, DXGI, or backend-native handles.

**Architecture:** Keep gameplay on `mirakana::GameApp`, `mirakana::IRenderer`, and `DesktopGameRunner`. Add a small `mirakana_runtime_host_sdl3_presentation` target with a `SdlDesktopPresentation` class that owns renderer selection diagnostics separately from the SDL event-pump target. It may inspect the private SDL window pointer from `SdlWindow::native_window()` inside the SDL3 host adapter, translate it to a first-party `rhi::SurfaceHandle` privately, and fall back to `NullRenderer` when the host cannot provide a D3D12-capable surface or runtime shader/pipeline policy is not ready. Do not add RHI dependencies to `mirakana_platform_sdl3`, and do not expose native handles to game code.

**Tech Stack:** C++23, SDL3 optional desktop GUI lane, `mirakana_runtime_host_sdl3_presentation`, `mirakana_platform_sdl3`, `mirakana_renderer`, `mirakana_rhi`, D3D12-ready first-party surface handles, NullRenderer fallback.

---

## File Structure

- Add `engine/runtime_host/sdl3/include/mirakana/runtime_host/sdl3/sdl_desktop_presentation.hpp`.
- Add `engine/runtime_host/sdl3/src/sdl_desktop_presentation.cpp`.
- Modify `engine/runtime_host/sdl3/CMakeLists.txt`.
- Modify `tests/unit/runtime_host_sdl3_tests.cpp`.
- Modify `games/sample_desktop_runtime_shell/main.cpp` and README/manifest to use the adapter.
- Sync `docs/roadmap.md`, `docs/rhi.md`, `docs/ai-game-development.md`, `docs/specs/2026-04-27-engine-essential-gap-analysis.md`, `engine/agent/manifest.json`, and game-development skills.

## Task 1: Host-Owned Presentation Contract

**Files:**
- Add: `engine/runtime_host/sdl3/include/mirakana/runtime_host/sdl3/sdl_desktop_presentation.hpp`
- Modify: `tests/unit/runtime_host_sdl3_tests.cpp`

- [x] **Step 1: Add failing tests first**

Add SDL dummy-driver tests that construct `SdlDesktopPresentation` from an `SdlWindow`, assert the selected backend is `null_renderer`, assert an `IRenderer` is available, and assert diagnostics explain that the native D3D12 surface or native renderer path was not selected.

- [x] **Step 2: Keep public API first-party**

Expose only `mirakana::` types: `SdlWindow`, `IRenderer`, `Extent2D`, enum statuses, and diagnostic strings. Do not expose `SDL_Window`, `HWND`, `IDXGI*`, or D3D12 symbols in the header.

## Task 2: SDL3 Surface Extraction And Null Fallback

**Files:**
- Add: `engine/runtime_host/sdl3/src/sdl_desktop_presentation.cpp`
- Modify: `engine/runtime_host/sdl3/CMakeLists.txt`

- [x] **Step 1: Implement adapter PIMPL**

Implement `SdlDesktopPresentation` with private ownership of a renderer and diagnostics. Use SDL3 window properties in the `.cpp` file to probe Win32 HWND when available; keep the result as an adapter-private first-party surface candidate.

- [x] **Step 2: Implement deterministic fallback**

If the platform surface is unavailable, D3D12 is not compiled into the current target, or runtime shader/pipeline policy is not wired, create a `NullRenderer` with the requested extent and record diagnostics. If fallback is disabled, throw a clear exception.

## Task 3: Sample Runtime Shell Integration

**Files:**
- Modify: `games/sample_desktop_runtime_shell/main.cpp`
- Modify: `games/sample_desktop_runtime_shell/README.md`
- Modify: `games/sample_desktop_runtime_shell/game.agent.json`

- [x] **Step 1: Route sample renderer selection through adapter**

Replace direct `NullRenderer` construction with `SdlDesktopPresentation` so the sample is already on the future native RHI renderer selection path while keeping the smoke run deterministic on dummy SDL.

- [x] **Step 2: Print backend diagnostics in smoke output**

Include the selected renderer backend in the sample output and make diagnostics visible to host users without changing gameplay code.

## Task 4: Documentation And Manifest Sync

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/rhi.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`

- [x] **Step 1: Register the active slice**

Set this plan as the active slice while implementation is underway, then move it to completed after validation.

- [x] **Step 2: Update runtime guidance**

Document that `mirakana_runtime_host_sdl3` now owns the presentation-selection boundary and Null fallback diagnostics, while true D3D12 shader-backed game-window rendering remains the next slice.

## Task 5: Verification

- [x] Run targeted `desktop-gui` runtime host tests.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record any host/toolchain blockers explicitly.

Validation notes:

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` passed the runtime host tests, SDL3 event-pump/presentation tests, and finite `sample_desktop_runtime_shell --smoke`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed, confirming the new public presentation header does not expose SDL3, Win32, DXGI, D3D12, or other native handles.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed. Diagnostic-only blockers remain host/toolchain-gated: Vulkan SPIR-V needs DXC SPIR-V CodeGen and `spirv-val`, Metal needs `metal`/`metallib`, Apple packaging needs macOS/Xcode tools, and strict tidy analysis needs a compile database for the `dev` preset.
