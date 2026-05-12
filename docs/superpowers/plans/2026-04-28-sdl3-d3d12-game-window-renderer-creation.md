# SDL3 D3D12 Game Window Renderer Creation Implementation Plan (2026-04-28)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extend the SDL3 desktop game presentation adapter so a host can supply precompiled D3D12 shader bytecode and receive a swapchain-backed `RhiFrameRenderer` for a game window, while preserving deterministic `NullRenderer` fallback.

**Architecture:** Keep game code on `mirakana::GameApp`, `DesktopGameRunner`, and `IRenderer`. Add host-only D3D12 renderer creation inputs to `SdlDesktopPresentationDesc` using first-party RHI value types and byte spans, not native handles or editor types. The adapter will create a D3D12 `IRhiDevice`, swapchain, empty pipeline layout, shader modules, graphics pipeline, and `RhiFrameRenderer` only when a real Win32 SDL surface, D3D12 runtime support, and non-empty shader bytecode are present; otherwise it will record a stable diagnostic and keep the existing `NullRenderer` fallback.

**Tech Stack:** C++23, SDL3 optional desktop GUI lane, `MK_runtime_host_sdl3_presentation`, `MK_renderer`, `MK_rhi`, `MK_rhi_d3d12`, Direct3D 12 precompiled shader bytecode, `RhiFrameRenderer`.

---

## File Structure

- Modify `engine/runtime_host/sdl3/include/mirakana/runtime_host/sdl3/sdl_desktop_presentation.hpp`.
- Modify `engine/runtime_host/sdl3/src/sdl_desktop_presentation.cpp`.
- Modify `tests/unit/runtime_host_sdl3_tests.cpp`.
- Modify `games/sample_desktop_runtime_shell/README.md` and `game.agent.json` if guidance changes.
- Sync `docs/roadmap.md`, `docs/rhi.md`, `docs/ai-game-development.md`, `docs/specs/2026-04-27-engine-essential-gap-analysis.md`, `engine/agent/manifest.json`, and game-development skills.

## Task 1: Host-Supplied D3D12 Shader Contract

**Files:**
- Modify: `engine/runtime_host/sdl3/include/mirakana/runtime_host/sdl3/sdl_desktop_presentation.hpp`
- Modify: `tests/unit/runtime_host_sdl3_tests.cpp`

- [x] **Step 1: Add failing tests for missing bytecode fallback**

Add a dummy-driver test that passes a D3D12 renderer request with empty vertex/fragment bytecode and asserts:

- `presentation.backend() == SdlDesktopPresentationBackend::null_renderer`
- first diagnostic reason is `SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable`
- reason name is `runtime_pipeline_unavailable`

- [x] **Step 2: Add public first-party bytecode descriptors**

Add:

```cpp
struct SdlDesktopPresentationShaderBytecode {
    std::string_view entry_point;
    std::span<const std::uint8_t> bytecode;
};

struct SdlDesktopPresentationD3d12RendererDesc {
    SdlDesktopPresentationShaderBytecode vertex_shader;
    SdlDesktopPresentationShaderBytecode fragment_shader;
    rhi::PrimitiveTopology topology{rhi::PrimitiveTopology::triangle_list};
    std::vector<rhi::VertexBufferLayoutDesc> vertex_buffers;
    std::vector<rhi::VertexAttributeDesc> vertex_attributes;
};
```

Then add `const SdlDesktopPresentationD3d12RendererDesc* d3d12_renderer{nullptr};` to `SdlDesktopPresentationDesc`.

The header may include `mirakana/rhi/rhi.hpp` because these are first-party RHI value types. It must not include SDL3, Windows, DXGI, D3D12, editor, or shader tool headers.

## Task 2: D3D12 Renderer Creation Path

**Files:**
- Modify: `engine/runtime_host/sdl3/src/sdl_desktop_presentation.cpp`

- [x] **Step 1: Validate explicit renderer requests before surface probing**

If `prefer_d3d12` is true and `d3d12_renderer != nullptr` but either shader bytecode span is empty or either entry point is empty, return `runtime_pipeline_unavailable` with a stable diagnostic and avoid native backend creation. If `d3d12_renderer == nullptr`, preserve the previous behavior: probe the surface first so dummy-driver fallback still reports `native_surface_unavailable`; if a real surface exists but no renderer request was supplied, report `runtime_pipeline_unavailable`.

- [x] **Step 2: Add native creation helper**

Behind `MK_RUNTIME_HOST_SDL3_PRESENTATION_HAS_D3D12`, implement a helper that:

1. probes the SDL3 Win32 surface,
2. checks D3D12 runtime support,
3. creates `rhi::d3d12::create_rhi_device({prefer_warp, enable_debug_layer})`,
4. creates a swapchain with `Format::bgra8_unorm`, buffer count `2`, requested `vsync`, and the probed surface,
5. creates vertex/fragment shaders from supplied bytecode,
6. creates an empty pipeline layout,
7. creates a graphics pipeline with supplied topology and vertex input metadata,
8. stores the device and creates `RhiFrameRenderer` using the swapchain and pipeline.

If any step throws or fails, capture a `runtime_pipeline_unavailable` diagnostic and fall back unless fallback is disabled.

- [x] **Step 3: Preserve ownership order**

`Impl` must own `std::unique_ptr<rhi::IRhiDevice> device;` before `std::unique_ptr<IRenderer> renderer;` in declaration order so the renderer is destroyed before the device it references.

## Task 3: Tests And Sample Contract

**Files:**
- Modify: `tests/unit/runtime_host_sdl3_tests.cpp`
- Modify: `games/sample_desktop_runtime_shell/README.md`
- Modify: `games/sample_desktop_runtime_shell/game.agent.json`

- [x] **Step 1: Keep dummy fallback tests stable**

Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` and confirm the dummy-driver fallback tests and sample smoke pass.

- [x] **Step 2: Document shader-backed opt-in**

Update sample docs and game manifest to say the sample currently does not ship shader bytecode, but host code can now opt into the D3D12 `RhiFrameRenderer` path by providing precompiled shader bytecode through `SdlDesktopPresentationDesc`.

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

Set this plan as Active while implementing and move it to Completed after validation.

- [x] **Step 2: Update readiness claims**

Docs must claim only that the adapter can create a native D3D12 `RhiFrameRenderer` when shader bytecode and a Win32 SDL surface are supplied. Do not claim the sample ships visible D3D12 rendering until an interactive/non-dummy smoke path with bytecode is validated.

## Task 5: Verification

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record diagnostic-only blockers explicitly.

## Validation Evidence

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`: passed; runtime host tests, SDL3 event-pump/presentation tests, and `sample_desktop_runtime_shell --smoke` passed.
- `MK_ENABLE_SDL3_D3D12_PRESENTATION_TEST=1 MK_runtime_host_sdl3_tests.exe`: passed; the env-gated real Windows SDL3/D3D12 bytecode smoke created a `d3d12` `RhiFrameRenderer` and submitted one frame.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: passed. Diagnostic-only blockers remain for Vulkan SPIR-V tooling (`dxc` SPIR-V CodeGen and `spirv-val` missing), Metal tooling (`metal`/`metallib` missing), Apple packaging (`macOS/Xcode` unavailable on this host), and strict tidy analysis before the dev compile database exists.
