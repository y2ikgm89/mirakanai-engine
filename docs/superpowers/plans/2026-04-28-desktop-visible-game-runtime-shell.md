# Desktop Visible Game Runtime Shell Implementation Plan (2026-04-28)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add the first editor-independent desktop game runtime shell path so a `games/<name>` executable can run `mirakana::GameApp` through a window-oriented host without exposing native SDL/OS/GPU handles to game code.

**Architecture:** Keep existing `mirakana::GameApp` and `mirakana::HeadlessRunner` unchanged. Add a small `mirakana_runtime_host` module that depends on public core/platform/renderer contracts and drives `IWindow`, virtual input, lifecycle, and optional renderer resize through a `DesktopGameRunner`. Add an optional `mirakana_runtime_host_sdl3` adapter that owns `SDL_PollEvent` behind a public engine event-pump class, then expose a `games/sample_desktop_runtime_shell` target only in the optional desktop GUI build.

**Tech Stack:** C++23, CMake target-based modules, `mirakana_core`, `mirakana_platform`, `mirakana_renderer`, optional `mirakana_platform_sdl3`/SDL3, first-party test harness, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

---

## File Structure

- Create `engine/runtime_host/include/mirakana/runtime_host/desktop_runner.hpp` for the host-independent desktop runner contract.
- Create `engine/runtime_host/src/desktop_runner.cpp` for lifecycle, window-close, input-reset, lifecycle-reset, and renderer-resize behavior.
- Create `engine/runtime_host/CMakeLists.txt` for `mirakana_runtime_host` and optional `sdl3` subdirectory wiring.
- Create `engine/runtime_host/sdl3/include/mirakana/runtime_host/sdl3/sdl_desktop_event_pump.hpp` and `engine/runtime_host/sdl3/src/sdl_desktop_event_pump.cpp` for SDL3 event polling without exposing SDL headers to game code.
- Create `engine/runtime_host/sdl3/CMakeLists.txt` for `mirakana_runtime_host_sdl3`.
- Create `tests/unit/runtime_host_tests.cpp` for host-independent runner coverage.
- Create `tests/unit/runtime_host_sdl3_tests.cpp` for optional dummy-driver SDL event-pump coverage.
- Create `games/sample_desktop_runtime_shell/README.md`, `main.cpp`, and `game.agent.json` for a windowed-null-renderer desktop shell sample.
- Modify `CMakeLists.txt`, `games/CMakeLists.txt`, install/export target lists, docs, and `engine/agent/manifest.json` to expose the new capability honestly.

## Task 1: Host-Independent Runner Test

**Files:**
- Create: `tests/unit/runtime_host_tests.cpp`
- Modify: `CMakeLists.txt`

- [x] **Step 1: Write the failing test**

Add tests that include `<mirakana/runtime_host/desktop_runner.hpp>`, use `HeadlessWindow`, `NullRenderer`, virtual inputs, and a fake `IDesktopEventPump`, then assert:

- `DesktopGameRunner` drives `GameApp` lifecycle until app stop.
- the runner returns `window_closed` when the event pump closes the window before update.
- the runner returns `lifecycle_quit` when the pump pushes a quit lifecycle event.
- renderer extent follows the window extent when `resize_renderer_to_window` is enabled.

- [x] **Step 2: Run the test to verify it fails**

Run `cmake --preset dev` after adding the test target. Expected: configure or build fails because `mirakana_runtime_host` and `mirakana/runtime_host/desktop_runner.hpp` do not exist yet.

## Task 2: Implement `mirakana_runtime_host`

**Files:**
- Create: `engine/runtime_host/include/mirakana/runtime_host/desktop_runner.hpp`
- Create: `engine/runtime_host/src/desktop_runner.cpp`
- Create: `engine/runtime_host/CMakeLists.txt`
- Modify: `CMakeLists.txt`

- [x] **Step 1: Add public contract**

Define `DesktopHostServices`, `DesktopRunConfig`, `DesktopRunStatus`, `DesktopRunResult`, `IDesktopEventPump`, and `DesktopGameRunner` in namespace `ge`. Keep the contract in terms of `IWindow`, `IRenderer`, `VirtualInput`, `VirtualPointerInput`, `VirtualGamepadInput`, `VirtualLifecycle`, `ILogger`, `Registry`, and `GameApp`.

- [x] **Step 2: Add minimal implementation**

Validate non-null `window`, positive fixed delta, run `on_start`/`on_update`/`on_stop`, reset per-frame input/lifecycle transient state before pumping events, stop on window close or lifecycle quit/terminate, and optionally resize the renderer to match the window extent.

- [x] **Step 3: Wire CMake**

Add `mirakana_runtime_host`, link it publicly to `mirakana_core`, `mirakana_platform`, and `mirakana_renderer`, and include it in install/export target lists.

- [x] **Step 4: Run tests**

Run `cmake --preset dev`, `cmake --build --preset dev`, and `ctest --preset dev --output-on-failure` or `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

## Task 3: Optional SDL3 Event Pump And Windowed Sample

**Files:**
- Create: `engine/runtime_host/sdl3/include/mirakana/runtime_host/sdl3/sdl_desktop_event_pump.hpp`
- Create: `engine/runtime_host/sdl3/src/sdl_desktop_event_pump.cpp`
- Create: `engine/runtime_host/sdl3/CMakeLists.txt`
- Create: `tests/unit/runtime_host_sdl3_tests.cpp`
- Create: `games/sample_desktop_runtime_shell/README.md`
- Create: `games/sample_desktop_runtime_shell/main.cpp`
- Create: `games/sample_desktop_runtime_shell/game.agent.json`
- Modify: `games/CMakeLists.txt`
- Modify: `CMakeLists.txt`

- [x] **Step 1: Add SDL3 adapter**

Implement `SdlDesktopEventPump` as an `IDesktopEventPump` that owns the `SDL_PollEvent` loop in the adapter source file and forwards translated events through `SdlWindow::handle_event`.

- [x] **Step 2: Add optional test**

Use the SDL dummy video driver and `SDL_PushEvent` to prove the event pump turns `SDL_EVENT_QUIT` into `VirtualLifecycle` quit state and closes the `SdlWindow`.

- [x] **Step 3: Add sample**

Add `sample_desktop_runtime_shell` as an optional desktop GUI target. It should include only public `mirakana::` headers, instantiate `SdlRuntime`, `SdlWindow`, `NullRenderer`, virtual input/lifecycle services, `SdlDesktopEventPump`, and `DesktopGameRunner`, then exit after a finite frame cap.

## Task 4: Documentation And Manifest Synchronization

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `docs/architecture.md`
- Modify: `docs/testing.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `docs/specs/game-prompt-pack.md`
- Modify: `docs/specs/game-template.md`
- Modify: `engine/agent/manifest.json`

- [x] **Step 1: Record capability**

Document that the first slice is a windowed desktop shell and SDL3 event pump with a NullRenderer fallback; native D3D12/Vulkan/Metal visible game presentation remains a follow-up backend binding slice.

- [x] **Step 2: Update AI guidance**

Expose `mirakana_runtime_host` and optional `mirakana_runtime_host_sdl3`, add a desktop game runtime packaging/validation target, and add a generated-game scenario that agents can select only when building the optional desktop GUI lane.

## Task 5: Verification

**Files:**
- No direct file edits.

- [x] **Step 1: Run default validation**

Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`. Expected: default build, tests, JSON/AI/dependency checks pass or report a concrete local tool blocker.

- [x] **Step 2: Run optional desktop GUI validation if dependencies are present**

Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` when local SDL3/vcpkg dependencies are available. Expected: optional SDL3 adapter, editor, and desktop runtime host tests build and run. If dependencies are absent, report the blocker explicitly.
