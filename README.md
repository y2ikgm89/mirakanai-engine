# MIRAIKANAI Engine

MIRAIKANAI Engine is published as `Mirakanai` in CMake/module namespaces and repository structure.

Branding and naming:

- Brand: `MIRAIKANAI` (`MIRAIKANAI Engine`)
- Repository/package identity: `mirakanai`
- C++ namespace: `mirakana::`
- CMake / target naming: installable package targets are `mirakana::` components (`mirakana::core`, `mirakana::rhi`, etc.). Local build/test targets are intentionally `MK_*` aliases.

A clean C++23 game engine built for AI-assisted development, desktop authoring, and future desktop/mobile runtimes.

## Current Stage

The `core-first-mvp` scope is closed as a verified MVP foundation, not as a complete commercial game engine. The closure record is `docs/superpowers/plans/2026-05-01-core-first-mvp-closure.md`; current status and next-phase work live in `docs/roadmap.md`.

The closed MVP foundation establishes:

- C++23 project structure
- CMake presets
- CI, release packaging, sanitizer, clang-format, and dependency-policy command foundations
- self-contained core module
- headless application loop
- minimal ECS primitives
- deterministic fixed timestep helper
- logging interface
- platform contracts for virtual key, pointer, and gamepad input, display/DPI/safe-area metadata, monitor selection and window placement policy, mobile lifecycle/touch/orientation/safe-area/storage/permission state, text clipboard access, asynchronous native-style file dialogs, cursor visibility/confinement/relative-input state, application lifecycle events, headless windows, in-memory/rooted filesystems, shell-free process command validation, recording process runs, and Windows stdout/stderr process execution
- RHI contracts with a GPU-free NullRhi backend for buffers, textures, swapchains, shaders, descriptor sets, copy/upload/readback commands with explicit texture footprints, CPU buffer readback, transient resources, presentation, graphics pipelines, render passes, draw calls, and fences
- renderer contracts with `NullRenderer`, `RhiFrameRenderer` for submitting renderer frames through an `IRhiDevice`, and `RhiViewportSurface` for renderer-owned offscreen viewport targets, renderer-submitted frames, and CPU readback frames
- `MK_scene_renderer` bridge for turning renderer-neutral scene packets into renderer mesh, sprite, camera matrix, and light command data, including material definition and material instance base-color resolution, sprite tint extraction, and rotation-aware transform extraction, without coupling scene or renderer internals
- Windows-only Direct3D 12 backend scaffold with DXGI/D3D12 runtime probe, device bootstrap validation, committed resource checks, PIMPL device/resource/command list ownership, CPU fence waits, cross-queue synchronization, HWND swapchain presentation, RTV ownership, back-buffer transitions, standalone texture transitions, tracked texture state validation, requested-color clears with non-black readback proof, shader-visible descriptor heaps, root signatures, descriptor table binding, native CBV/SRV/UAV descriptor view writes, shader bytecode ownership, graphics PSO ownership, swapchain and texture render-target setup, first triangle draw recording with texture readback proof, and an initial `IRhiDevice` bridge for backend-neutral resources, descriptor sets, shaders, pipelines, buffer copies, aligned buffer/texture copies, transient leases, surface-backed swapchains, queued presentation, swapchain and texture render passes, descriptor set binding, draw recording, command submission, and fence wait ownership
- tests without third-party dependencies
- shader source metadata and generated artifact tracking contracts
- shader compiler command planning for DXC, Vulkan SPIR-V, and Metal tools, shell-free shader tool runner validation, compiler output capture contracts, include dependency discovery, shader include dependency graph edges, shader-tool process execution adapter with artifact output policy and reviewed executable override, deterministic shader toolchain discovery with DXC SPIR-V CodeGen readiness, shader artifact provenance/cache invalidation, persistent shader cache index storage, deterministic artifact marker writes for tests, real artifact enforcement for compiler-backed runs, and deterministic shader artifact manifest serialization
- texture, mesh, audio, and material import metadata contracts, first-party texture/mesh/audio source document decoding, deterministic asset import planning, filesystem-backed import execution for cooked artifacts, filesystem-backed hot reload snapshot scanning, hot reload snapshot tracking, and material definition/material instance serialization and composition
- scene components for cameras, lights, mesh renderers, and sprite renderers, renderer-neutral render packet extraction with rotation-aware world transforms, scene-subtree prefab creation, plus prefab instantiation
- deterministic first-party foundations for 2D/3D physics integration, 3D AABB contact detection/resolution, audio mix planning, keyframe animation sampling, animation state machines, timeline events, mouse/touch-style pointer input, and gamepad input
- first-party `MK_ui` retained-mode HUD/menu contracts with deterministic headless row/column/stack layout solving and renderer submission payload construction, plus `MK_ui_renderer` for theme-token UI box submission through `MK_renderer`
- Android/iOS package templates with explicit local SDK/toolchain blocker diagnostics through `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-mobile-packaging.ps1`
- Codex/AI workflow guidance
- license and provenance rules
- vcpkg manifest dependency policy with an official registry `builtin-baseline` for reproducible optional GUI dependencies
- optional SDL3 + Dear ImGui desktop editor shell
- GUI-independent editor project document with shader tool settings, project settings draft validation, project creation wizard, project/workspace migration, project bundle IO, workspace serialization, content browser state, asset import queue state with tool execution result mapping and imported asset record registration helpers, asset hot reload event state, shader compile queue/status/execution result state, viewport shader artifact readiness state, viewport state, viewport run controls, viewport tool state, scene transform drafts, scene component drafts, viewport transform edit application, command registry, command palette model, dirty tracking, and undo/redo history
- deterministic scene serialization and render packet extraction for editor/runtime scene exchange, including camera, light, mesh renderer, and sprite renderer components

Apple-hosted iOS simulator/device validation, backend-neutral Metal visible presentation, production material/shader graph authoring, live shader generation, production UI text/font/IME/accessibility/image adapters, richer postprocess/shadow systems, GPU skinning/upload, advanced physics/navigation/audio systems, telemetry/crash reporting, allocator diagnostics, and GPU marker adapters remain next-phase or host-gated roadmap work.

## Requirements

- **PowerShell 7** (`pwsh`) for repository automation scripts under `tools/`
- CMake 3.30+
- A C++23-capable compiler
- Windows SDK when building the Windows D3D12 backend/tests
- On Windows, use Visual Studio Developer PowerShell/Command Prompt for direct `cmake` commands, or install official CMake 3.30+ on `PATH`. Repository wrappers resolve and report the selected CMake/CTest tools through `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1`.
- Direct `clang-format ...` commands require `clang-format` on `PATH`. Repository `format` wrappers resolve and report official LLVM or Visual Studio Build Tools LLVM tools through `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1`.

Optional Windows diagnostics use official Microsoft tools: Debugging Tools for Windows (`cdb`, `windbg`), Windows Graphics Tools for the D3D12 debug layer, PIX on Windows for D3D12 captures, and Windows Performance Toolkit (`wpr`, `wpa`, `xperf`) for ETW/performance work. These are host-local diagnostics, not default build dependencies.

## Validate

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

The validation script runs static repository checks first, then a CMake/CTest/Visual Studio toolchain preflight, then CMake configure/build/test if the required C++ tools are available. It does not require Ninja; CMake may choose the default generator for the local environment. The PowerShell wrapper normalizes duplicate `Path`/`PATH` child-process entries before launching build tools so MSBuild can start `CL.exe` reliably from Codex/terminal environments.

Static checks include AI integration, JSON contracts, license/provenance records, and dependency policy for the pinned vcpkg manifest.

## Quality Commands

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1
```

`tools/package.ps1` builds the Release preset, runs Release tests, installs the SDK, validates `examples/installed_consumer` against the installed `mirakana::` CMake package, and creates a CPack ZIP. `check-format.ps1` and `format.ps1` use the official LLVM `clang-format` tool when it is installed locally, including Visual Studio Build Tools LLVM tools even when `clang-format` is not on `PATH`.

## Optional Desktop Editor

The default build remains third-party-free. To build the optional desktop editor shell, bootstrap GUI dependencies through vcpkg and build the GUI preset:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1
```

This builds and tests `MK_platform_sdl3` plus `MK_editor`, an SDL3 + Dear ImGui docking editor shell with SDL display/DPI/safe-area reporting, first-party monitor selection and window placement policy, SDL text clipboard access, SDL asynchronous native file dialog access, SDL cursor visibility/grab/relative mode control, SDL application lifecycle event mapping, SDL keyboard/mouse/touch/gamepad input mapping, project creation, filterable Assets, import queue, `Import Assets` execution through the first-party texture/mesh/audio/material asset import tool with successful imports reflected back into the Content Browser, hot reload status, shader compile status and a `Compile Shader` action wired through editable project shader tool settings and the reviewed process runner path, shader tool discovery, editable viewport render backend preference, an RHI-backed scene Viewport submitted through `build_scene_render_packet` and `MK_scene_renderer`, copied through `RhiViewportReadbackFrame` into an SDL display texture for `ImGui::Image`, Viewport toolbar, transform and camera/light/mesh/sprite component controls, Play/Pause/Stop controls, Scene, Inspector, Console, Profiler, Project Settings, command palette, scene edit, and undo/redo surfaces.

## AI Agent Context

Codex, Claude Code, and other coding agents should read the machine-readable engine context before generating game code or changing engine APIs:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1
```

The context is built from `engine/agent/manifest.json`, repository docs, public headers, skills, and subagent definitions.

## Create a Game

AI agents and humans can scaffold a new C++ game project with:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/new-game.ps1 -Name my-game
```

Games are stored under `games/<name>/`, include a `game.agent.json` contract, and are built through `MK_add_game` in `games/CMakeLists.txt`.

Current samples include `sample_headless`, `sample_input_renderer`, `sample_gameplay_foundation`, and `sample_ui_audio_assets`.

Installed SDK validation can be run after a release install with:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-installed-sdk.ps1
```

## Intended Build Loop

Use the repository wrappers for repeatable validation and toolchain diagnostics:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1
```

Direct CMake commands are supported when the active shell has CMake on `PATH`. On Windows, launch Visual Studio Developer PowerShell/Command Prompt first, or install official CMake 3.30+ on `PATH`. To enforce that direct-CMake precondition, run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1 -RequireDirectCMake
cmake --preset dev
cmake --build --preset dev
ctest --preset dev --output-on-failure
```

Direct `clang-format --dry-run ...` commands have the same PATH requirement. If `check-toolchain.ps1` reports `direct-clang-format-status=unavailable`, use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` / `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` or add the reported `clang-format` directory to `PATH`.

## C++ Conventions

Naming, source layout, public include paths, and CMake package target rules are defined in `docs/cpp-style.md`.

## C++ Standard

The engine requires C++23. The standard policy and local toolchain notes are recorded in `docs/cpp-standard.md`.

MSVC builds select their C++23 mode through the centralized `MK_MSVC_CXX23_STANDARD_OPTION` CMake cache value. It currently uses `/std:c++23preview`; when MSVC/CMake provide the stable official `/std:c++23` path, switch that value to `/std:c++23`. C++ module scanning is enabled, and `import std;` is gated on CMake-reported toolchain support.

C++23 verification can be run explicitly with:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/evaluate-cpp23.ps1
```

## License

The project is currently all rights reserved under `LicenseRef-Proprietary`. Third-party code and assets must be recorded before they are introduced.

