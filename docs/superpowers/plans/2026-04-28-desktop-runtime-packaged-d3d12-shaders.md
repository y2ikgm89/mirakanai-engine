# Desktop Runtime Packaged D3D12 Shaders Implementation Plan (2026-04-28)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Let the editor-independent desktop runtime sample load packaged precompiled D3D12 shader bytecode and opt into the SDL3 D3D12 `RhiFrameRenderer` path without exposing native handles to game code.

**Architecture:** Add a host-independent shader bytecode loading helper to `mirakana_runtime_host` that reads non-empty packaged bytecode through `IFileSystem` and returns owned byte vectors plus entry points. Keep shader compilation out of runtime code: the sample builds DXIL artifacts into the executable output directory only when `dxc` is found at build time, and otherwise keeps the existing `NullRenderer` fallback. `sample_desktop_runtime_shell` remains a host executable; gameplay still sees only `mirakana::GameApp`, `IRenderer`, and public host contracts.

**Tech Stack:** C++23, `mirakana_runtime_host`, `mirakana_platform` `IFileSystem`, optional SDL3 desktop GUI lane, Windows SDK `dxc` as a build-time tool, `mirakana_runtime_host_sdl3_presentation`, D3D12 precompiled shader bytecode.

---

## File Structure

- Create `engine/runtime_host/include/mirakana/runtime_host/shader_bytecode.hpp`.
- Create `engine/runtime_host/src/shader_bytecode.cpp`.
- Modify `engine/runtime_host/CMakeLists.txt`.
- Modify `tests/unit/runtime_host_tests.cpp`.
- Create `games/sample_desktop_runtime_shell/shaders/runtime_shell.hlsl`.
- Modify `games/sample_desktop_runtime_shell/main.cpp`.
- Modify `games/CMakeLists.txt`.
- Sync `docs/superpowers/plans/README.md`, `docs/roadmap.md`, `docs/rhi.md`, `docs/ai-game-development.md`, `docs/specs/2026-04-27-engine-essential-gap-analysis.md`, `engine/agent/manifest.json`, and game-development skills.

## Task 1: Host-Independent Bytecode Loader

**Files:**
- Modify: `tests/unit/runtime_host_tests.cpp`
- Create: `engine/runtime_host/include/mirakana/runtime_host/shader_bytecode.hpp`
- Create: `engine/runtime_host/src/shader_bytecode.cpp`
- Modify: `engine/runtime_host/CMakeLists.txt`

- [x] **Step 1: Add failing loader tests**

Add tests that prove:

- non-empty binary-like strings from `MemoryFileSystem` load into `std::vector<std::uint8_t>` without losing bytes,
- missing vertex or fragment bytecode returns `missing`,
- empty vertex or fragment bytecode returns `empty`,
- filesystem read exceptions return `read_failed` with a stable diagnostic,
- a null filesystem or empty entry point returns `invalid_request`.

- [x] **Step 2: Implement `load_desktop_shader_bytecode_pair`**

Add first-party public structs in `mirakana::`:

```cpp
enum class DesktopShaderBytecodeLoadStatus {
    ready = 0,
    missing,
    empty,
    read_failed,
    invalid_request,
};

struct DesktopShaderBytecodeBlob {
    std::string entry_point;
    std::vector<std::uint8_t> bytecode;
};

struct DesktopShaderBytecodeLoadDesc {
    IFileSystem* filesystem{nullptr};
    std::string vertex_path;
    std::string fragment_path;
    std::string vertex_entry_point{"vs_main"};
    std::string fragment_entry_point{"ps_main"};
};

struct DesktopShaderBytecodeLoadResult {
    DesktopShaderBytecodeLoadStatus status{DesktopShaderBytecodeLoadStatus::invalid_request};
    DesktopShaderBytecodeBlob vertex_shader;
    DesktopShaderBytecodeBlob fragment_shader;
    std::string diagnostic;

    [[nodiscard]] bool ready() const noexcept;
};

[[nodiscard]] DesktopShaderBytecodeLoadResult load_desktop_shader_bytecode_pair(
    const DesktopShaderBytecodeLoadDesc& desc);
[[nodiscard]] std::string_view desktop_shader_bytecode_load_status_name(
    DesktopShaderBytecodeLoadStatus status) noexcept;
```

The helper must use `IFileSystem`, not native filesystem APIs, so tests stay deterministic. It must not include SDL3, D3D12, DXGI, Win32, or editor headers.

## Task 2: Sample Runtime Shell Integration

**Files:**
- Modify: `games/sample_desktop_runtime_shell/main.cpp`
- Create: `games/sample_desktop_runtime_shell/shaders/runtime_shell.hlsl`
- Modify: `games/CMakeLists.txt`

- [x] **Step 1: Add sample command options**

Add:

- `--require-d3d12-shaders`: fail the sample after loading if packaged VS/PS bytecode is unavailable.
- `--require-d3d12-renderer`: fail the sample after presentation creation if the selected backend is not `d3d12`.

Keep `--smoke` deterministic with the dummy driver unless the caller explicitly passes `--video-driver`.

- [x] **Step 2: Load packaged shader artifacts before presentation creation**

Resolve the executable directory from `argv[0]`, create a `RootedFileSystem`, and load:

- `shaders/runtime_shell.vs.dxil`
- `shaders/runtime_shell.ps.dxil`

When the loader returns `ready`, populate `SdlDesktopPresentationD3d12RendererDesc` with spans into the owned vectors before constructing `SdlDesktopPresentation`. When not ready, print a shader diagnostic and keep existing fallback behavior.

- [x] **Step 3: Add a minimal HLSL shader source**

The source should use `SV_VertexID` for a procedural triangle and one pixel shader output. Do not add runtime shader compilation.

- [x] **Step 4: Add build-time DXIL generation when `dxc` is available**

On Windows, find `dxc` from PATH or installed Windows SDK directories. If found, post-build compile the HLSL source to the target output directory under `shaders/`. Add a CTest smoke that runs:

```powershell
sample_desktop_runtime_shell --smoke --require-d3d12-shaders
```

only when this build-time shader generation is configured.

## Task 3: Documentation And Manifest Sync

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/rhi.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`
- Modify: `games/sample_desktop_runtime_shell/README.md`
- Modify: `games/sample_desktop_runtime_shell/game.agent.json`

- [x] **Step 1: Register active slice**

Set this plan as Active during implementation and move it to Completed after validation.

- [x] **Step 2: Update readiness claims**

Docs must claim only that the sample can load build-output packaged DXIL when `dxc` is found and can require those artifacts in CTest. Release package installation of generated shader artifacts and Vulkan/Metal sample presentation remain follow-up work.

## Task 4: Verification

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`.
- [x] Run the real-window sample smoke when DXIL artifacts exist: `sample_desktop_runtime_shell --smoke --video-driver windows --require-d3d12-shaders --require-d3d12-renderer`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record diagnostic-only blockers explicitly.

## Verification Results

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1`: passed, 18/18 default tests, including `read_failed` filesystem exception coverage.
- DXIL regeneration check: after deleting `out/build/desktop-gui/games/Debug/shaders/runtime_shell.{vs,ps}.dxil`, `cmake --build --preset desktop-gui --target sample_desktop_runtime_shell` regenerated both artifacts through the CMake build graph.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`: passed with 4/4 focused tests after sandbox escalation for vcpkg tool extraction; includes `sample_desktop_runtime_shell_shader_artifacts_smoke`.
- Real-window smoke: `sample_desktop_runtime_shell --smoke --video-driver windows --require-d3d12-shaders --require-d3d12-renderer` passed with `renderer=d3d12`, 2 frames.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`: D3D12 DXIL ready; Vulkan SPIR-V CodeGen, `spirv-val`, `metal`, and `metallib` are diagnostic-only blockers.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: passed. Diagnostic-only blockers remain Apple packaging on non-macOS/Xcode hosts, Android signing/device availability, Vulkan/Metal shader tooling, and strict tidy compile database availability.
