# Generic Desktop Runtime Game Host Implementation Plan (2026-04-28)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the SDL3 desktop runtime path reusable by any `games/<name>` executable without copying the one-off `sample_desktop_runtime_shell` host assembly.

**Architecture:** Add a small `mirakana::SdlDesktopGameHost` contract in the optional SDL3 desktop runtime host presentation module. The host owns SDL runtime/window setup, `SdlDesktopPresentation`, virtual input/pointer/gamepad/lifecycle state, event pumping, and `DesktopGameRunner` service wiring while exposing only first-party `mirakana::` contracts to game code. Keep shader bytecode compilation and packaged artifact policy host-owned and optional; `sample_desktop_runtime_shell` remains the proof executable and package target for this slice.

**Tech Stack:** C++23, target-based CMake, SDL3 optional desktop runtime lane, `mirakana_runtime_host`, `mirakana_runtime_host_sdl3`, `mirakana_runtime_host_sdl3_presentation`, existing CTest and PowerShell validation lanes.

---

## File Structure

- Create `engine/runtime_host/sdl3/include/mirakana/runtime_host/sdl3/sdl_desktop_game_host.hpp` for the reusable host-facing launcher contract.
- Create `engine/runtime_host/sdl3/src/sdl_desktop_game_host.cpp` for SDL runtime/window/presentation/event-pump ownership and `DesktopGameRunner` service wiring.
- Modify `engine/runtime_host/sdl3/CMakeLists.txt` so `mirakana_runtime_host_sdl3_presentation` builds and exports the new host contract and links `mirakana_runtime_host_sdl3`.
- Modify `tests/unit/runtime_host_sdl3_tests.cpp` with a dummy-driver smoke test for the reusable host.
- Modify `games/sample_desktop_runtime_shell/main.cpp` to use the reusable host while keeping sample CLI, shader artifact loading, diagnostics, exit codes, and `GameApp` behavior intact.
- Modify `docs/superpowers/plans/README.md`, `docs/roadmap.md`, `docs/ai-game-development.md`, `docs/specs/2026-04-27-engine-essential-gap-analysis.md`, `engine/agent/manifest.json`, `.agents/skills/gameengine-game-development/SKILL.md`, `.claude/skills/gameengine-game-development/SKILL.md`, and `.claude/agents/gameplay-builder.md` to describe the new reusable host path honestly.

## Task 1: Reusable Host Contract Test

**Files:**
- Modify: `tests/unit/runtime_host_sdl3_tests.cpp`

- [x] **Step 1: Write the failing host smoke test**

Add a test that includes `mirakana/runtime_host/sdl3/sdl_desktop_game_host.hpp`, constructs `mirakana::SdlDesktopGameHost` with the SDL dummy video driver and a non-zero window extent, builds a small `mirakana::GameApp` from `host.input()` and `host.renderer()`, runs it for two frames through `host.run(...)`, and asserts:

- `DesktopRunStatus::completed`
- `frames_run == 2`
- app update count is `2`
- renderer backend is `"null"` on the dummy driver
- renderer finished two frames
- presentation diagnostics are non-empty

- [x] **Step 2: Verify RED**

Run:

```powershell
cmake --build --preset desktop-runtime --target mirakana_runtime_host_sdl3_tests
```

Expected before implementation: compile failure because `sdl_desktop_game_host.hpp` and `mirakana::SdlDesktopGameHost` do not exist.

## Task 2: Implement `mirakana::SdlDesktopGameHost`

**Files:**
- Create: `engine/runtime_host/sdl3/include/mirakana/runtime_host/sdl3/sdl_desktop_game_host.hpp`
- Create: `engine/runtime_host/sdl3/src/sdl_desktop_game_host.cpp`
- Modify: `engine/runtime_host/sdl3/CMakeLists.txt`

- [x] **Step 1: Add the public first-party host descriptor**

Create `SdlDesktopGameHostDesc` with:

- `std::string title`
- `WindowExtent extent`
- `std::string video_driver_hint`
- presentation policy fields: `prefer_d3d12`, `allow_null_fallback`, `prefer_warp`, `enable_debug_layer`, `vsync`
- optional `const SdlDesktopPresentationD3d12RendererDesc* d3d12_renderer`
- optional `ILogger* logger`
- optional `Registry* registry`
- `std::size_t default_log_capacity`

No SDL headers, native OS handles, GPU handles, or editor types may appear in this header.

- [x] **Step 2: Add the host class**

Create `SdlDesktopGameHost` with:

- constructor from `SdlDesktopGameHostDesc`
- `IWindow& window()`
- `IRenderer& renderer()`
- `VirtualInput& input()`
- `VirtualPointerInput& pointer_input()`
- `VirtualGamepadInput& gamepad_input()`
- `VirtualLifecycle& lifecycle()`
- `SdlDesktopPresentationBackend presentation_backend()`
- `std::string_view presentation_backend_name()`
- `std::span<const SdlDesktopPresentationDiagnostic> presentation_diagnostics()`
- `DesktopRunResult run(GameApp& app, DesktopRunConfig config = {})`

The implementation owns `SdlRuntime`, `SdlWindow`, `SdlDesktopPresentation`, `SdlDesktopEventPump`, default `RingBufferLogger`, default `Registry`, and virtual input/lifecycle state.

- [x] **Step 3: Wire CMake**

Add the new source file to `mirakana_runtime_host_sdl3_presentation` and link that target publicly to `mirakana_runtime_host_sdl3` so installed consumers get the event-pump contract transitively when using the game host.

- [x] **Step 4: Verify GREEN**

Run:

```powershell
cmake --build --preset desktop-runtime --target mirakana_runtime_host_sdl3_tests
ctest --preset desktop-runtime --output-on-failure -R mirakana_runtime_host_sdl3_tests
```

Expected after implementation: the new test and existing SDL3 runtime host tests pass.

## Task 3: Adapt The Sample Shell

**Files:**
- Modify: `games/sample_desktop_runtime_shell/main.cpp`

- [x] **Step 1: Replace one-off host wiring**

Keep sample CLI parsing, shader bytecode loading, required-shader exit code `4`, required-renderer exit code `5`, smoke exit code `3`, and stdout diagnostics unchanged in meaning. Replace direct construction of `SdlRuntime`, `SdlWindow`, `SdlDesktopPresentation`, `SdlDesktopEventPump`, `DesktopHostServices`, and `DesktopGameRunner` with `mirakana::SdlDesktopGameHost`.

- [x] **Step 2: Verify sample target**

Run:

```powershell
cmake --build --preset desktop-runtime --target sample_desktop_runtime_shell
ctest --preset desktop-runtime --output-on-failure -R "sample_desktop_runtime_shell(_shader_artifacts)?_smoke"
```

Expected: finite smoke still exits cleanly and the optional shader artifact smoke still passes when DXIL artifacts are available.

## Task 4: Documentation And Agent Contract Sync

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/agents/gameplay-builder.md`

- [x] **Step 1: Register this slice**

Mark this plan as the active slice during implementation and move it to completed after validation.

- [x] **Step 2: Update reusable host guidance**

State that games selecting `desktop-game-runtime` can use `mirakana::SdlDesktopGameHost` for the optional SDL3 desktop host path, while gameplay remains `mirakana::GameApp` and must not expose SDL3, native OS, GPU, RHI backend, Dear ImGui, or editor APIs.

- [x] **Step 3: Keep packaging claims narrow**

State that `desktop-runtime-release` still packages the validated `sample_desktop_runtime_shell` proof executable in this slice. Broader multi-game runtime package selection remains follow-up work unless a later slice implements generic package target selection.

## Task 5: Verification

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` because a public host header is added.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1` because the packaged sample executable now uses the reusable host.
- [x] Record any diagnostic-only blockers explicitly.

## Verification Results

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` initially failed after the test include was added because `mirakana/runtime_host/sdl3/sdl_desktop_game_host.hpp` did not exist.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` passed after implementing `mirakana::SdlDesktopGameHost`, including `mirakana_runtime_host_tests`, `mirakana_runtime_host_sdl3_tests`, SDL3 platform/audio tests, `sample_desktop_runtime_shell_smoke`, and `sample_desktop_runtime_shell_shader_artifacts_smoke`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: passed. Diagnostic-only blockers remain Vulkan SPIR-V toolchain readiness (`dxc` SPIR-V CodeGen and `spirv-val` missing), Metal toolchain readiness (`metal`/`metallib` missing), Apple/Xcode packaging on this Windows host, optional Android signing/device state, and strict tidy compile database availability.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1`: passed. The Release package lane built and installed the sample executable, SDL3 runtime DLL, generated DXIL artifacts, installed SDK consumer validation, installed sample smoke with `--require-d3d12-shaders`, and generated `out/build/desktop-runtime-release/GameEngine-0.1.0-Windows-AMD64.zip`.
- Rendering audit subagent found no handle-boundary or layer-boundary issue. It noted a low-risk existing gap: packaged DXIL is only proven through the manual real-window D3D12 smoke path, while the automated dummy smoke proves artifact loading and fallback-safe execution.
