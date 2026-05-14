# Core-First MVP Design

Historical note: this document records the initial MVP design. The current `core-first-mvp` scope is closed as a verified MVP foundation by `docs/superpowers/plans/2026-05-01-core-first-mvp-closure.md`; use `docs/roadmap.md` and `engine/agent/manifest.json` for current capabilities.

## Goal

Create the initial clean C++23 game engine foundation for AI-assisted development.

## Non-Goals

- No real renderer backend.
- No mobile app packaging.
- No fully featured editor workflow.
- No third-party dependencies in the default headless build.

## Architecture

The MVP creates:

- `mirakana_core` CMake target
- `mirakana_sandbox` example executable
- `mirakana_core_tests` CTest executable
- repository-level Codex instructions
- repository skills and project subagent definitions
- legal and provenance documentation

The core module contains:

- `version`: compile-time project version constants
- `log`: logger interface and ring buffer logger
- `time`: deterministic fixed timestep helper
- `entity`: entity id and hashing
- `registry`: minimal component registry
- `application`: headless game app lifecycle runner
- `mirakana_assets`: logical asset ids, asset records, registry, asset dependency graph, hot reload snapshot/event tracking, shader source metadata, generated shader artifact tracking, shader compiler command planning, shell-free shader tool runner validation, compiler output capture contracts, shader include dependency discovery, shader include dependency graph edge generation, shader artifact manifest serialization, texture/mesh/audio/material import metadata, first-party texture/mesh/audio source document decoding, asset import planning, material definition serialization, and material instance serialization/composition
- `mirakana_animation`: keyframe validation and clamped interpolation
- `mirakana_audio`: device-independent audio mix planning
- `mirakana_math`: vector, matrix, and rotation-aware transform primitives usable by 2D and 3D games
- `mirakana_platform`: `VirtualInput`, `VirtualPointerInput`, `VirtualGamepadInput`, display/DPI/safe-area contracts through `DisplayInfo` and `WindowDisplayState`, monitor selection policy through `DisplaySelectionRequest` and `select_display`, window placement policy through `WindowPlacementRequest` and `plan_window_placement`, text clipboard contracts through `IClipboard` and `MemoryClipboard`, asynchronous native-style file dialog contracts through `FileDialogRequest`, `IFileDialogService`, and `MemoryFileDialogService`, cursor state contracts through `CursorMode`, `ICursor`, and `MemoryCursor`, application lifecycle contracts through `VirtualLifecycle`, `HeadlessWindow`, `MemoryFileSystem`, `RootedFileSystem`, shell-free `ProcessCommand` validation, recording process runs, and Windows stdout/stderr process execution for deterministic platform tests and tool execution
- `mirakana_physics`: deterministic 2D body integration
- `mirakana_tools`: shader tool process adapter that maps shader compile commands to platform process execution with artifact output-root policy and reviewed executable override, deterministic shader toolchain discovery, shader compile actions with provenance writes, persistent cache index storage, cache-hit reuse, deterministic test artifact marker writes, real artifact enforcement for compiler-backed runs, filesystem-backed texture/mesh/audio/material import execution for deterministic cooked artifacts, and filesystem-backed asset snapshot scanning for hot reload
- `mirakana_renderer`: renderer contracts plus `NullRenderer` for GPU-free validation, `RhiFrameRenderer` for RHI-backed frame submission, and `RhiViewportSurface` for renderer-owned offscreen viewport targets with CPU readback frames
- `mirakana_scene_renderer`: bridge from scene render packets to renderer mesh, sprite, camera matrix, and light command data with material definition/material instance base-color resolution, sprite tint mapping, and transform extraction, without coupling scene and renderer modules
- `mirakana_rhi`: graphics API independent hardware interface contracts plus `NullRhiDevice`, including descriptor set layout/binding, copy/upload/readback footprint, CPU buffer readback, transient resource, and presentation validation
- `mirakana_rhi_d3d12`: Windows-only Direct3D 12 compile, runtime probe, device bootstrap, committed resource ownership, and PIMPL device/command list/fence/swapchain/RTV/descriptor heap/root signature/shader/PSO context scaffold with CPU waits, GPU-side cross-queue waits, HWND presentation, back-buffer transitions, standalone texture transitions, clears, native CBV/SRV/UAV descriptor view writes, descriptor table binding, swapchain and texture render-target setup, buffer and aligned buffer/texture copy recording, first triangle draw recording, and an initial `IRhiDevice` bridge for backend-neutral resources, descriptor sets, shaders, pipelines, buffer copies, aligned buffer/texture copies, transient leases, surface-backed swapchains, queued presentation, swapchain and texture render passes, descriptor set binding, draw recording, command submission, and fence wait ownership
- `mirakana_scene`: named scene nodes, rotation-aware transforms, hierarchy, camera/light/mesh/sprite renderer components, renderer-neutral render packet extraction with world transforms, scene-subtree prefab creation, and prefab instantiation
- `mirakana_platform_sdl3`: optional SDL3 runtime/window backend with display enumeration, first-party monitor selection and window placement policy, window display scale, pixel density, safe-area reporting, text clipboard access, asynchronous native file dialog access, cursor visibility/grab/relative mode control, application lifecycle event mapping, and keyboard, mouse, touch, and gamepad input mapping for desktop GUI builds
- `mirakana_editor_core`: GUI-independent editor project document with shader tool settings, project settings draft validation, project creation wizard, project/workspace migration, shader artifact manifest storage, workspace serialization, panel state, content browser state, asset import queue state, asset import execution result mapping, imported asset record registration helpers, asset hot reload event state, shader compile queue/status state, shader compile execution result mapping, viewport state, viewport run controls, viewport tool state, and command registry
- `mirakana_editor`: optional Dear ImGui desktop editor shell with project creation, project bundle IO, asset browser, import queue, `Import Assets` tool execution with successful imports reflected in the Content Browser, hot reload event display, shader compile status display, shader tool discovery, editable viewport render backend and shader tool Project Settings, project-setting-driven `Compile Shader` process-runner action, RHI-backed viewport surface updates, CPU readback into an SDL viewport display texture, viewport toolbar, and play controls

## Public API

The API is namespaced under `ge`. At this historical milestone public headers lived first in `engine/core/include/mirakana/core/`; the current engine now exposes module-specific public headers listed in `engine/agent/manifest.json`.

## Data Flow

Game code implements `mirakana::GameApp`. A `mirakana::HeadlessRunner` owns the lifecycle sequence:

```text
on_start -> repeated on_update -> on_stop
```

The runner passes an `EngineContext` containing a logger and registry.

## Error Handling

The MVP avoids exceptions in the main lifecycle result. `HeadlessRunner::run` returns `RunResult` with an exit status and frame count.

## Testing

The project uses a first-party test harness and CTest. Tests validate deterministic behavior, public API expectations, math operations, rotation-aware transform composition, virtual input, platform display/DPI/safe-area, clipboard, asynchronous file dialog, cursor, and lifecycle contracts, process/filesystem execution contracts, tool adapter contracts, shader toolchain discovery/provenance/cache index/action behavior, renderer lifecycle state, scene renderer bridge transform extraction, camera matrix extraction, light command extraction, and material/material-instance color resolution behavior, RHI resource/descriptor/copy footprint/CPU readback/transient/present/pipeline/render pass/fence contracts, viewport readback contracts, asset registry/texture-mesh-audio source-format/material/import/hot-reload-scan behavior, scene component/render-packet/prefab/serialization behavior, physics/audio/animation foundations, and editor IO/command/palette/history/project creation/project shader tool settings/project settings draft/asset pipeline execution mapping/imported asset registration/shader compile state/execution result contracts.

## Legal

All first-party files use `LicenseRef-Proprietary`. No third-party distributable material is included in the MVP.
