# Core-First MVP Implementation Plan (2026-04-25)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the initial C++23 core-first game engine repository foundation.

**Architecture:** Create a standard-library-only `MK_core` target with deterministic runtime primitives, a sandbox executable, and a self-contained unit test executable. Keep platform and renderer work out of the MVP while documenting their future boundaries.

**Tech Stack:** C++23, CMake, CTest, PowerShell 7 scripts under `tools/`, Codex AGENTS.md, repository skills.

---

### Task 1: Repository Controls

**Files:**
- Create: `AGENTS.md`
- Create: `README.md`
- Create: `package.json`
- Create: `.gitignore`

- [x] Define project-wide AI instructions.
- [x] Add PowerShell 7 `tools/*.ps1` validation entrypoints.
- [x] Document the core-first stage.

### Task 2: Architecture and Legal Docs

**Files:**
- Create: `docs/architecture.md`
- Create: `docs/legal-and-licensing.md`
- Create: `docs/testing.md`
- Create: `docs/workflows.md`
- Create: `docs/adr/0001-core-first-cpp-engine.md`
- Create: `docs/specs/2026-04-25-core-first-mvp-design.md`
- Create: `LICENSES/LicenseRef-Proprietary.txt`
- Create: `THIRD_PARTY_NOTICES.md`

- [x] Record core-first architecture.
- [x] Record license policy.
- [x] Record validation policy.

### Task 3: CMake Bootstrap

**Files:**
- Create: `CMakeLists.txt`
- Create: `CMakePresets.json`

- [x] Define `MK_core`.
- [x] Define `MK_sandbox`.
- [x] Define `MK_core_tests`.

### Task 4: Core Tests

**Files:**
- Create: `tests/test_framework.hpp`
- Create: `tests/unit/core_tests.cpp`

- [x] Add tests for version constants.
- [x] Add tests for fixed timestep behavior.
- [x] Add tests for logging behavior.
- [x] Add tests for entity and component registry behavior.
- [x] Add tests for headless runner lifecycle.

### Task 5: Core Implementation

**Files:**
- Create: `engine/core/include/mirakana/core/version.hpp`
- Create: `engine/core/src/version.cpp`
- Create: `engine/core/include/mirakana/core/log.hpp`
- Create: `engine/core/src/log.cpp`
- Create: `engine/core/include/mirakana/core/time.hpp`
- Create: `engine/core/src/time.cpp`
- Create: `engine/core/include/mirakana/core/entity.hpp`
- Create: `engine/core/include/mirakana/core/registry.hpp`
- Create: `engine/core/src/registry.cpp`
- Create: `engine/core/include/mirakana/core/application.hpp`
- Create: `engine/core/src/application.cpp`

- [x] Implement standard-library-only core primitives.
- [x] Keep template component storage in public headers.
- [x] Keep non-template functions in source files.

### Task 6: Validation Scripts

**Files:**
- Create: `tools/check-license.ps1`
- Create: `tools/build.ps1`
- Create: `tools/test.ps1`
- Create: `tools/validate.ps1`

- [x] Add static license/header checks.
- [x] Add CMake build/test wrappers.
- [x] Make missing CMake/compiler errors explicit.

### Task 7: Verify

**Commands:**
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`

**Expected:**
- Static repository checks pass.
- CMake build/test run when local C++ tools are installed.
- If tools are missing, validation fails with a specific missing-tool message.

### Task 8: AI Game Development Surface

**Files:**
- Create: `engine/agent/manifest.json`
- Create: `docs/ai-game-development.md`
- Create: `games/sample_headless/game.agent.json`
- Create: `tools/new-game.ps1`
- Modify: `CMakeLists.txt`

- [x] Add AI-readable engine manifest.
- [x] Add game project manifest convention.
- [x] Add new game scaffold command.
- [x] Register games as CTest executables.

### Task 9: Math and Null Renderer

**Files:**
- Create: `engine/math/include/mirakana/math/vec.hpp`
- Create: `engine/math/include/mirakana/math/mat4.hpp`
- Create: `engine/math/include/mirakana/math/transform.hpp`
- Create: `engine/math/src/math.cpp`
- Create: `engine/renderer/include/mirakana/renderer/renderer.hpp`
- Create: `engine/renderer/src/null_renderer.cpp`
- Modify: `tests/unit/core_tests.cpp`
- Modify: `CMakeLists.txt`

- [x] Write failing tests for math and renderer API.
- [x] Fix validation scripts so native build failures stop validation.
- [x] Implement math primitives.
- [x] Implement renderer interface and `NullRenderer`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

### Task 10: Headless Input Contract

**Files:**
- Create: `engine/platform/include/mirakana/platform/input.hpp`
- Create: `engine/platform/src/input.cpp`
- Modify: `tests/unit/core_tests.cpp`
- Modify: `CMakeLists.txt`
- Modify: `engine/agent/manifest.json`

- [x] Write failing tests for virtual key pressed/down/released states.
- [x] Write failing tests for digital movement axis.
- [x] Implement `MK_platform` and `mirakana::VirtualInput`.
- [x] Write failing tests for pointer press/move/release frame deltas, deterministic pointer ordering, and invalid pointer sample handling.
- [x] Implement `mirakana::VirtualPointerInput` for mouse, touch, and pen-style pointer state.
- [x] Write failing tests for gamepad button frame states, axis clamping, deterministic device ordering, and invalid gamepad samples.
- [x] Implement `mirakana::VirtualGamepadInput` for device-independent gamepad buttons and axes.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

### Task 11: Renderer Draw Commands

**Files:**
- Modify: `engine/renderer/include/mirakana/renderer/renderer.hpp`
- Modify: `engine/renderer/src/null_renderer.cpp`
- Modify: `tests/unit/core_tests.cpp`
- Modify: `games/sample_input_renderer/main.cpp`

- [x] Write failing tests for 2D/3D draw command submission counts.
- [x] Add `SpriteCommand` and `MeshCommand`.
- [x] Count draw submissions in `NullRenderer`.
- [x] Use `draw_sprite` from the input/renderer sample game.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

### Task 12: Asset Registry

**Files:**
- Create: `engine/assets/include/mirakana/assets/asset_registry.hpp`
- Create: `engine/assets/src/asset_registry.cpp`
- Modify: `tests/unit/core_tests.cpp`
- Modify: `CMakeLists.txt`
- Modify: `engine/agent/manifest.json`

- [x] Write failing tests for logical asset registration and duplicate rejection.
- [x] Add `AssetId`, `AssetKind`, `AssetRecord`, and `AssetRegistry`.
- [x] Register `MK_assets` as a CMake module.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

### Task 13: Platform Window and Filesystem Contracts

**Files:**
- Create: `engine/platform/include/mirakana/platform/window.hpp`
- Create: `engine/platform/src/window.cpp`
- Create: `engine/platform/include/mirakana/platform/filesystem.hpp`
- Create: `engine/platform/src/filesystem.cpp`
- Modify: `tests/unit/core_tests.cpp`
- Modify: `engine/platform/CMakeLists.txt`

- [x] Write failing tests for `HeadlessWindow`.
- [x] Write failing tests for `MemoryFileSystem`.
- [x] Implement platform-free contracts.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

### Task 14: Scene Foundation

**Files:**
- Create: `engine/scene/include/mirakana/scene/scene.hpp`
- Create: `engine/scene/include/mirakana/scene/render_packet.hpp`
- Create: `engine/scene/src/scene.cpp`
- Create: `engine/scene/src/render_packet.cpp`
- Create: `engine/scene/CMakeLists.txt`
- Modify: `tests/unit/core_tests.cpp`
- Modify: `CMakeLists.txt`

- [x] Write failing tests for scene node creation and parenting.
- [x] Implement `MK_scene`.
- [x] Add renderer-neutral scene render packet extraction for visible meshes, cameras, lights, primary-camera lookup, and world transforms.
- [x] Add radians-based 2D/3D transform rotation, scene rotation serialization, and rotation-aware scene render packet world transforms.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

### Task 14.1: Scene Renderer Bridge

**Files:**
- Create: `engine/scene_renderer/include/mirakana/scene_renderer/scene_renderer.hpp`
- Create: `engine/scene_renderer/src/scene_renderer.cpp`
- Create: `engine/scene_renderer/CMakeLists.txt`
- Create: `tests/unit/scene_renderer_tests.cpp`
- Modify: `CMakeLists.txt`

- [x] Write failing tests for scene packet to renderer command conversion.
- [x] Implement `MK_scene_renderer`.
- [x] Add scene material palette resolution from `MaterialDefinition` base-color factors.
- [x] Add position, scale, and z-rotation extraction from scene world matrices.
- [x] Register the bridge target, install headers, and expose it to game targets.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

### Task 15: Editor Scene Transform Editing

**Files:**
- Create: `editor/core/include/mirakana/editor/scene_edit.hpp`
- Create: `editor/core/src/scene_edit.cpp`
- Modify: `editor/src/main.cpp`
- Modify: `editor/CMakeLists.txt`
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Write failing tests for scene transform drafts and viewport transform edit deltas.
- [x] Implement GUI-independent selected-node transform editing in `MK_editor_core`.
- [x] Wire Inspector position/rotation/scale controls through the editor-core draft contract.
- [x] Wire Viewport toolbar axis delta controls through active Move/Rotate/Scale tools.
- [x] Preserve selected node and route scene replacements through undo/redo.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 16: Renderer-Submitted Editor Viewport

**Files:**
- Modify: `engine/renderer/include/mirakana/renderer/rhi_viewport_surface.hpp`
- Modify: `engine/renderer/src/rhi_viewport_surface.cpp`
- Modify: `editor/src/main.cpp`
- Modify: `editor/CMakeLists.txt`
- Modify: `tests/unit/renderer_rhi_tests.cpp`

- [x] Write failing tests for viewport frames submitted through `IRenderer`.
- [x] Add `RhiViewportSurface::render_frame` with explicit render-target/copy-source transitions.
- [x] Wire editor viewport submission through `build_scene_render_packet` and `MK_scene_renderer`.
- [x] Add default editor camera, light, mesh renderer, and material palette data.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 17: Editor Scene Component Editing

**Files:**
- Modify: `editor/core/include/mirakana/editor/scene_edit.hpp`
- Modify: `editor/core/src/scene_edit.cpp`
- Modify: `editor/src/main.cpp`
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Write failing tests for camera, light, and mesh renderer component drafts.
- [x] Add GUI-independent scene component draft creation and validated apply helpers.
- [x] Wire Inspector component add/remove and field editing through scene replacement.
- [x] Keep component edits in undo/redo and preserve selected node.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 18: Scene Subtree Prefab Creation

**Files:**
- Modify: `engine/scene/include/mirakana/scene/prefab.hpp`
- Modify: `engine/scene/src/prefab.cpp`
- Modify: `tests/unit/core_tests.cpp`

- [x] Write failing tests for deterministic prefab creation from a scene subtree.
- [x] Add `build_prefab_from_scene_subtree`.
- [x] Preserve transform, component, and parent-index data in deterministic traversal order.
- [x] Reject missing roots and invalid prefab names without mutation.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

### Task 19: Editor Render Backend Selection

**Files:**
- Create: `editor/core/include/mirakana/editor/render_backend.hpp`
- Create: `editor/core/src/render_backend.cpp`
- Modify: `editor/core/include/mirakana/editor/project.hpp`
- Modify: `editor/core/src/project.cpp`
- Modify: `editor/core/include/mirakana/editor/viewport.hpp`
- Modify: `editor/core/src/viewport.cpp`
- Modify: `editor/src/main.cpp`
- Modify: `editor/CMakeLists.txt`
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Write failing tests for project render backend persistence, backend availability selection, and viewport backend state.
- [x] Add GUI-independent `EditorRenderBackend` preference, descriptor, availability, and host-policy selection helpers.
- [x] Persist project render backend preference through `GameEngine.Project.v3` and migrate older projects to `auto`.
- [x] Wire Project Settings to edit the requested viewport backend and show the resolved active backend.
- [x] Keep the current visible editor viewport on `NullRhiDevice` until D3D12/Vulkan/Metal interop is verified.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

### Task 20: Editor Shader Tool Discovery

**Files:**
- Create: `editor/core/include/mirakana/editor/shader_tool_discovery.hpp`
- Create: `editor/core/src/shader_tool_discovery.cpp`
- Modify: `editor/core/include/mirakana/editor/project.hpp`
- Modify: `editor/core/src/project.cpp`
- Modify: `editor/src/main.cpp`
- Modify: `editor/CMakeLists.txt`
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Write failing tests for discovered shader tool UI state and draft application.
- [x] Add `ShaderToolDiscoveryState` for deterministic `dxc`/`metal`/`metallib` options.
- [x] Add `ProjectSettingsDraft::set_shader_tool_descriptor`.
- [x] Wire Project Settings to refresh discovery and copy a discovered executable into the shader tool draft.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

### Task 21: Conditional D3D12 Editor Viewport Bridge

**Files:**
- Modify: `editor/src/main.cpp`
- Modify: `editor/CMakeLists.txt`

- [x] Link the Windows GUI editor target to `MK_rhi_d3d12` behind `MK_EDITOR_ENABLE_D3D12`.
- [x] Compile both editor vertex and pixel shader requests from the default HLSL source.
- [x] Treat D3D12 as an editor viewport candidate only when real VS/PS DXIL artifacts exist and the D3D12 runtime probe succeeds.
- [x] Create the selected D3D12 `IRhiDevice` bridge and pipeline from artifact bytecode, falling back to `NullRhiDevice` if native creation fails.
- [x] Keep `RhiViewportReadbackFrame` as the deterministic fallback; the current excellence plan has since added Windows D3D12 shared-texture editor display interop.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 22: Editor Viewport Shader Artifact Readiness

**Files:**
- Create: `editor/core/include/mirakana/editor/viewport_shader_artifacts.hpp`
- Create: `editor/core/src/viewport_shader_artifacts.cpp`
- Modify: `editor/src/main.cpp`
- Modify: `editor/CMakeLists.txt`
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Write failing tests for missing, empty, ready, and unsupported viewport shader artifacts.
- [x] Add GUI-independent `ViewportShaderArtifactState` for non-empty D3D12 vertex/fragment bytecode readiness.
- [x] Use artifact readiness for Windows D3D12 editor viewport availability and bytecode loading.
- [x] Display viewport shader readiness in the Viewport and Project Settings panels.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 23: Scene Sprite Renderer Path

**Files:**
- Modify: `engine/scene/include/mirakana/scene/components.hpp`
- Modify: `engine/scene/src/components.cpp`
- Modify: `engine/scene/include/mirakana/scene/render_packet.hpp`
- Modify: `engine/scene/src/render_packet.cpp`
- Modify: `engine/scene/src/scene_io.cpp`
- Modify: `engine/scene_renderer/include/mirakana/scene_renderer/scene_renderer.hpp`
- Modify: `engine/scene_renderer/src/scene_renderer.cpp`
- Modify: `editor/src/main.cpp`
- Modify: `tests/unit/core_tests.cpp`
- Modify: `tests/unit/scene_renderer_tests.cpp`
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Write failing tests for sprite renderer validation, scene serialization, render packet extraction, and scene renderer submission.
- [x] Add `SpriteRendererComponent` with sprite/material ids, size, tint, and visibility.
- [x] Serialize, deserialize, validate, and extract visible sprite renderers into `SceneRenderPacket`.
- [x] Convert scene sprites to `SpriteCommand` and submit them through `MK_scene_renderer`.
- [x] Expose Sprite Renderer add/remove, visibility, size, and tint controls in the Inspector.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 24: First-Party Audio Import Contract

**Files:**
- Modify: `engine/assets/include/mirakana/assets/asset_import_metadata.hpp`
- Modify: `engine/assets/src/asset_import_metadata.cpp`
- Modify: `engine/assets/include/mirakana/assets/asset_import_pipeline.hpp`
- Modify: `engine/assets/src/asset_import_pipeline.cpp`
- Modify: `engine/assets/include/mirakana/assets/asset_source_format.hpp`
- Modify: `engine/assets/src/asset_source_format.cpp`
- Modify: `engine/tools/src/asset_import_tool.cpp`
- Modify: `editor/core/src/asset_pipeline.cpp`
- Modify: `editor/src/main.cpp`
- Modify: `tests/unit/core_tests.cpp`
- Modify: `tests/unit/tools_tests.cpp`
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Write failing tests for audio import metadata, import plan actions, audio source document parsing, cooked audio artifact writes, and editor imported asset record mapping.
- [x] Add `AudioImportMetadata`, `AssetImportActionKind::audio`, and first-party `AudioSourceDocument` serialization/parsing.
- [x] Cook deterministic `GameEngine.CookedAudio.v1` artifacts through `execute_asset_import_plan`.
- [x] Map imported audio artifacts to `AssetKind::audio` records for the editor Content Browser.
- [x] Add the default editor audio asset to the editor import queue and source fixture writer.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 25: Material Instance Contract

**Files:**
- Modify: `engine/assets/include/mirakana/assets/material.hpp`
- Modify: `engine/assets/src/material.cpp`
- Modify: `engine/scene_renderer/include/mirakana/scene_renderer/scene_renderer.hpp`
- Modify: `engine/scene_renderer/src/scene_renderer.cpp`
- Modify: `tests/unit/core_tests.cpp`
- Modify: `tests/unit/scene_renderer_tests.cpp`

- [x] Write failing tests for material instance validation, deterministic serialization, parent material composition, and scene renderer palette color resolution.
- [x] Add `MaterialInstanceDefinition`, `compose_material_instance`, and `GameEngine.MaterialInstance.v1` serialization/parsing.
- [x] Preserve inherited material state while overriding factors and texture slots deterministically.
- [x] Add `SceneMaterialPalette::try_add_instance`/`add_instance` so render packet submission can resolve composed instance colors.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

### Task 26: Scene Camera and Light Render Data

**Files:**
- Modify: `engine/scene_renderer/include/mirakana/scene_renderer/scene_renderer.hpp`
- Modify: `engine/scene_renderer/src/scene_renderer.cpp`
- Modify: `tests/unit/scene_renderer_tests.cpp`

- [x] Write failing tests for camera view/projection matrix extraction and renderer-neutral light command extraction from scene render packets.
- [x] Add `SceneCameraMatrices` and `make_scene_camera_matrices` for perspective and orthographic camera components.
- [x] Add `SceneLightCommand` and `make_scene_light_command` for directional/point/spot light component data.
- [x] Keep camera and light render data in `MK_scene_renderer` so `MK_scene` remains renderer-neutral.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

### Task 27: RHI Backend Capability and Probe Contract

**Files:**
- Create: `engine/rhi/include/mirakana/rhi/backend_capabilities.hpp`
- Create: `engine/rhi/src/backend_capabilities.cpp`
- Modify: `engine/rhi/CMakeLists.txt`
- Modify: `tests/unit/rhi_tests.cpp`
- Modify: `docs/rhi.md`
- Modify: `docs/architecture.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/rendering-change/SKILL.md`
- Modify: `.claude/skills/gameengine-rendering/SKILL.md`

- [x] Write failing tests for backend capability profiles, host support policy, preferred backend order, Vulkan/Metal probe plans, and deterministic probe diagnostics.
- [x] Add API-independent `BackendCapabilityProfile`, `BackendProbePlan`, and `BackendProbeResult` helpers.
- [x] Encode Vulkan host/runtime/device/queue/shader artifact bootstrap assumptions from Khronos guidance.
- [x] Encode Metal host/runtime/default-device/command-queue/shader library bootstrap assumptions from Apple guidance.
- [x] Update RHI docs, architecture docs, agent manifest, and Codex/Claude rendering skills.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

### Task 28: Vulkan and Metal Backend Scaffold Targets

**Files:**
- Create: `engine/rhi/vulkan/CMakeLists.txt`
- Create: `engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_backend.hpp`
- Create: `engine/rhi/vulkan/src/vulkan_backend.cpp`
- Create: `engine/rhi/metal/CMakeLists.txt`
- Create: `engine/rhi/metal/include/mirakana/rhi/metal/metal_backend.hpp`
- Create: `engine/rhi/metal/src/metal_backend.cpp`
- Create: `tests/unit/backend_scaffold_tests.cpp`
- Modify: `engine/rhi/CMakeLists.txt`
- Modify: `CMakeLists.txt`
- Modify: `docs/rhi.md`
- Modify: `docs/architecture.md`
- Modify: `engine/agent/manifest.json`

- [x] Write failing tests for SDK-independent Vulkan and Metal backend scaffold identity, capability, host support, probe plan, and deterministic probe diagnostics.
- [x] Add `MK_rhi_vulkan` and `MK_rhi_metal` CMake targets without native SDK headers or public native handles.
- [x] Expose `mirakana::rhi::vulkan` and `mirakana::rhi::metal` wrappers over the shared backend capability/probe contract.
- [x] Install Vulkan/Metal scaffold public headers with the rest of the RHI API.
- [x] Update RHI docs, architecture docs, and agent manifest for the new backend modules.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

### Task 29: Vulkan Runtime Loader Probe

**Files:**
- Modify: `engine/rhi/include/mirakana/rhi/backend_capabilities.hpp`
- Modify: `engine/rhi/src/backend_capabilities.cpp`
- Modify: `engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_backend.hpp`
- Modify: `engine/rhi/vulkan/src/vulkan_backend.cpp`
- Modify: `engine/rhi/vulkan/CMakeLists.txt`
- Modify: `tests/unit/backend_scaffold_tests.cpp`
- Modify: `docs/rhi.md`
- Modify: `docs/architecture.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/rendering-change/SKILL.md`
- Modify: `.claude/skills/gameengine-rendering/SKILL.md`

- [x] Write failing tests for deterministic Vulkan loader probe result classification and current-host runtime probing without exposing native handles.
- [x] Add `current_rhi_host_platform` to the RHI backend capability contract.
- [x] Add `VulkanLoaderProbeDesc`, `VulkanLoaderProbeResult`, `default_runtime_library_name`, `make_loader_probe_result`, and `probe_runtime_loader`.
- [x] Probe `vulkan-1.dll` on Windows and `libvulkan.so.1` on Linux for `vkGetInstanceProcAddr` while keeping loader handles private.
- [x] Update RHI docs, architecture docs, agent manifest, and Codex/Claude rendering skills.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

### Task 30: Vulkan Physical Device Selection Contract

**Files:**
- Modify: `engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_backend.hpp`
- Modify: `engine/rhi/vulkan/src/vulkan_backend.cpp`
- Modify: `tests/unit/backend_scaffold_tests.cpp`
- Modify: `docs/rhi.md`
- Modify: `docs/architecture.md`
- Modify: `engine/agent/manifest.json`

- [x] Write failing tests for selecting a suitable discrete Vulkan device with graphics/present queue support and rejecting devices missing required API level, swapchain extension, dynamic rendering, or present queues.
- [x] Add SDK-independent `VulkanApiVersion`, `VulkanPhysicalDeviceCandidate`, `VulkanQueueFamilyCandidate`, and `VulkanDeviceSelection`.
- [x] Add `select_physical_device` so future native enumeration maps into deterministic first-party selection before logical device creation.
- [x] Document Vulkan 1.3, swapchain, dynamic rendering, and graphics/present queue requirements.
- [x] Update agent manifest for AI-facing Vulkan implementation status.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

### Task 31: Vulkan Instance Create Plan Contract

**Files:**
- Modify: `engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_backend.hpp`
- Modify: `engine/rhi/vulkan/src/vulkan_backend.cpp`
- Modify: `tests/unit/backend_scaffold_tests.cpp`
- Modify: `docs/rhi.md`
- Modify: `docs/architecture.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/rendering-change/SKILL.md`
- Modify: `.claude/skills/gameengine-rendering/SKILL.md`

- [x] Write failing tests for Vulkan instance API version, application name, required extension, optional extension, and debug-utils validation readiness planning.
- [x] Add SDK-independent `VulkanInstanceCreateDesc`, `VulkanInstanceCreatePlan`, and `build_instance_create_plan`.
- [x] Reject Vulkan API versions older than 1.3 and missing required instance extensions before future native `vkCreateInstance` wiring.
- [x] Preserve deterministic enabled extension ordering while enabling only available optional extensions.
- [x] Update RHI docs, architecture docs, agent manifest, and Codex/Claude rendering skills.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

### Task 32: Vulkan Logical Device Create Plan Contract

**Files:**
- Modify: `engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_backend.hpp`
- Modify: `engine/rhi/vulkan/src/vulkan_backend.cpp`
- Modify: `tests/unit/backend_scaffold_tests.cpp`
- Modify: `docs/rhi.md`
- Modify: `docs/architecture.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/rendering-change/SKILL.md`
- Modify: `.claude/skills/gameengine-rendering/SKILL.md`

- [x] Write failing tests for logical device queue family planning, queue family deduplication, required/optional device extension enablement, and dynamic rendering feature readiness.
- [x] Add SDK-independent `VulkanLogicalDeviceCreateDesc`, `VulkanDeviceQueueCreatePlan`, `VulkanLogicalDeviceCreatePlan`, and `build_logical_device_create_plan`.
- [x] Reject unsuitable device selections, missing required device extensions, missing swapchain support, and missing dynamic rendering support before future native `vkCreateDevice` wiring.
- [x] Preserve deterministic queue family and enabled device extension ordering.
- [x] Update RHI docs, architecture docs, agent manifest, and Codex/Claude rendering skills.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

### Task 33: Vulkan Command Entry Point Resolution Contract

**Files:**
- Modify: `engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_backend.hpp`
- Modify: `engine/rhi/vulkan/src/vulkan_backend.cpp`
- Modify: `tests/unit/backend_scaffold_tests.cpp`
- Modify: `docs/rhi.md`
- Modify: `docs/architecture.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/rendering-change/SKILL.md`
- Modify: `.claude/skills/gameengine-rendering/SKILL.md`

- [x] Write failing tests for loader/global/instance/device command request coverage and missing required command diagnostics.
- [x] Add SDK-independent `VulkanCommandScope`, `VulkanCommandRequest`, `VulkanCommandAvailability`, `VulkanCommandResolution`, `VulkanCommandResolutionPlan`, `vulkan_backend_command_requests`, and `build_command_resolution_plan`.
- [x] Cover required bootstrap, dynamic rendering, and swapchain command names without exposing native function pointers.
- [x] Allow optional debug-utils commands to be absent while rejecting missing required commands deterministically.
- [x] Update RHI docs, architecture docs, agent manifest, and Codex/Claude rendering skills.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

### Task 34: Vulkan Runtime Global Command Probe

**Files:**
- Modify: `engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_backend.hpp`
- Modify: `engine/rhi/vulkan/src/vulkan_backend.cpp`
- Modify: `tests/unit/backend_scaffold_tests.cpp`
- Modify: `docs/rhi.md`
- Modify: `docs/architecture.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/rendering-change/SKILL.md`
- Modify: `.claude/skills/gameengine-rendering/SKILL.md`

- [x] Write failing tests for current-host runtime global command probing without exposing native function pointers.
- [x] Add `VulkanRuntimeGlobalCommandProbeResult` and `probe_runtime_global_commands`.
- [x] Resolve loader/global command availability through `vkGetInstanceProcAddr(nullptr, ...)` after loading the platform Vulkan runtime.
- [x] Keep loader handles and native function pointers private to `engine/rhi/vulkan`.
- [x] Update RHI docs, architecture docs, agent manifest, and Codex/Claude rendering skills.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

### Task 35: Vulkan Runtime Instance Capability Probe

**Files:**
- Modify: `engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_backend.hpp`
- Modify: `engine/rhi/vulkan/src/vulkan_backend.cpp`
- Modify: `tests/unit/backend_scaffold_tests.cpp`
- Modify: `docs/rhi.md`
- Modify: `docs/architecture.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/rendering-change/SKILL.md`
- Modify: `.claude/skills/gameengine-rendering/SKILL.md`

- [x] Write failing tests for packed Vulkan API version decoding and current-host runtime instance capability probing without exposing native handles.
- [x] Add `VulkanRuntimeInstanceCapabilityProbeResult`, `decode_vulkan_api_version`, and `probe_runtime_instance_capabilities`.
- [x] Query `vkEnumerateInstanceVersion` and `vkEnumerateInstanceExtensionProperties` through private runtime-loaded function pointers.
- [x] Feed runtime extension names into `build_instance_create_plan` and reject runtimes older than the requested instance API version.
- [x] Update RHI docs, architecture docs, agent manifest, and Codex/Claude rendering skills.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 36: Vulkan Transient Instance Command Probe

**Files:**
- Modify: `engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_backend.hpp`
- Modify: `engine/rhi/vulkan/src/vulkan_backend.cpp`
- Modify: `tests/unit/backend_scaffold_tests.cpp`
- Modify: `docs/rhi.md`
- Modify: `docs/architecture.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/rendering-change/SKILL.md`
- Modify: `.claude/skills/gameengine-rendering/SKILL.md`

- [x] Write failing tests for Vulkan API version encoding and transient instance command probing without exposing native handles.
- [x] Add `VulkanRuntimeInstanceCommandProbeResult`, `encode_vulkan_api_version`, and `probe_runtime_instance_commands`.
- [x] Create a transient native instance through private `vkCreateInstance` ABI mirrors and destroy it through private `vkDestroyInstance` resolution.
- [x] Resolve instance-scope command availability through `vkGetInstanceProcAddr(instance, ...)` and feed `build_command_resolution_plan`.
- [x] Update RHI docs, architecture docs, agent manifest, and Codex/Claude rendering skills.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 37: Vulkan Physical Device Count Probe

**Files:**
- Modify: `engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_backend.hpp`
- Modify: `engine/rhi/vulkan/src/vulkan_backend.cpp`
- Modify: `tests/unit/backend_scaffold_tests.cpp`
- Modify: `docs/rhi.md`
- Modify: `docs/architecture.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/rendering-change/SKILL.md`
- Modify: `.claude/skills/gameengine-rendering/SKILL.md`

- [x] Write failing tests for count-only physical-device enumeration without exposing native handles.
- [x] Add `VulkanRuntimePhysicalDeviceCountProbeResult` and `probe_runtime_physical_device_count`.
- [x] Reuse the transient instance boundary and call `vkEnumeratePhysicalDevices` with `pPhysicalDevices == nullptr`.
- [x] Keep physical device handles and larger Vulkan property structs out of public APIs.
- [x] Update RHI docs, architecture docs, agent manifest, and Codex/Claude rendering skills.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 38: Vulkan Physical Device Snapshot Probe

**Files:**
- Modify: `engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_backend.hpp`
- Modify: `engine/rhi/vulkan/src/vulkan_backend.cpp`
- Modify: `tests/unit/backend_scaffold_tests.cpp`
- Modify: `docs/rhi.md`
- Modify: `docs/architecture.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/rendering-change/SKILL.md`
- Modify: `.claude/skills/gameengine-rendering/SKILL.md`

- [x] Write failing tests for physical-device snapshot enumeration without exposing native handles.
- [x] Add `VulkanRuntimePhysicalDeviceSnapshot`, `VulkanRuntimePhysicalDeviceSnapshotProbeResult`, and `probe_runtime_physical_device_snapshots`.
- [x] Reuse the transient instance boundary and enumerate private physical-device handles before destroying the instance.
- [x] Map `vkGetPhysicalDeviceQueueFamilyProperties` into first-party queue family candidates with present support left unknown until a surface probe exists.
- [x] Map `vkEnumerateDeviceExtensionProperties` into deterministic extension name lists and `VK_KHR_swapchain` support.
- [x] Keep native physical-device handles and large Vulkan property/feature structs out of public APIs.
- [x] Update RHI docs, architecture docs, agent manifest, and Codex/Claude rendering skills.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 39: Vulkan Dynamic Rendering Feature Snapshot

**Files:**
- Modify: `engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_backend.hpp`
- Modify: `engine/rhi/vulkan/src/vulkan_backend.cpp`
- Modify: `tests/unit/backend_scaffold_tests.cpp`
- Modify: `docs/rhi.md`
- Modify: `docs/architecture.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/rendering-change/SKILL.md`
- Modify: `.claude/skills/gameengine-rendering/SKILL.md`

- [x] Write failing tests that require runtime physical-device snapshots to expose first-party dynamic rendering support.
- [x] Add `supports_dynamic_rendering` to `VulkanRuntimePhysicalDeviceSnapshot`.
- [x] Add private ABI mirrors for `VkPhysicalDeviceFeatures2`, `VkPhysicalDeviceFeatures`, and `VkPhysicalDeviceDynamicRenderingFeatures` without adding public Vulkan headers.
- [x] Query `vkGetPhysicalDeviceFeatures2` for each private physical-device handle and map `dynamicRendering` into a first-party bool.
- [x] Keep command-name availability separate from feature support so future `select_physical_device` input is deterministic.
- [x] Update RHI docs, architecture docs, agent manifest, and Codex/Claude rendering skills.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 40: Vulkan Surface Present Support Probe

**Files:**
- Modify: `engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_backend.hpp`
- Modify: `engine/rhi/vulkan/src/vulkan_backend.cpp`
- Modify: `tests/unit/backend_scaffold_tests.cpp`
- Modify: `docs/rhi.md`
- Modify: `docs/architecture.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/rendering-change/SKILL.md`
- Modify: `.claude/skills/gameengine-rendering/SKILL.md`

- [x] Write failing tests for host surface extension planning and safe rejection of empty surface handles.
- [x] Add `VulkanRuntimeSurfaceSupportProbeResult`, `vulkan_surface_instance_extensions`, and `probe_runtime_surface_support`.
- [x] Add Win32 surface instance extension planning for `VK_KHR_surface` and `VK_KHR_win32_surface`, plus Android extension planning for future mobile packaging.
- [x] Create private Win32 `VkSurfaceKHR` handles through `vkCreateWin32SurfaceKHR` using first-party `SurfaceHandle` and destroy them through `vkDestroySurfaceKHR` before returning.
- [x] Query `vkGetPhysicalDeviceSurfaceSupportKHR` for queue families and map present support into first-party queue family snapshots.
- [x] Keep `VkSurfaceKHR`, `HWND`, native instance handles, and Vulkan function pointers out of public APIs.
- [x] Update RHI docs, architecture docs, agent manifest, and Codex/Claude rendering skills.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 41: Vulkan Physical Device Properties Snapshot

**Files:**
- Modify: `engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_backend.hpp`
- Modify: `engine/rhi/vulkan/src/vulkan_backend.cpp`
- Modify: `tests/unit/backend_scaffold_tests.cpp`
- Modify: `docs/rhi.md`
- Modify: `docs/architecture.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/rendering-change/SKILL.md`
- Modify: `.claude/skills/gameengine-rendering/SKILL.md`

- [x] Write failing tests requiring runtime physical-device snapshots to expose first-party properties for deterministic selection.
- [x] Add device name, type, API version, driver version, vendor id, and device id to `VulkanRuntimePhysicalDeviceSnapshot`.
- [x] Add `make_physical_device_candidate` to project snapshots into `VulkanPhysicalDeviceCandidate`.
- [x] Query `vkGetPhysicalDeviceProperties2` with private `VkPhysicalDeviceProperties2` mirrors and keep native handles/Vulkan headers out of public APIs.
- [x] Update RHI docs, architecture docs, agent manifest, and Codex/Claude rendering skills.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 42: Vulkan Runtime Physical Device Selection Probe

**Files:**
- Modify: `engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_backend.hpp`
- Modify: `engine/rhi/vulkan/src/vulkan_backend.cpp`
- Modify: `tests/unit/backend_scaffold_tests.cpp`
- Modify: `docs/rhi.md`
- Modify: `docs/architecture.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/rendering-change/SKILL.md`
- Modify: `.claude/skills/gameengine-rendering/SKILL.md`

- [x] Write failing tests requiring runtime snapshots to project into physical-device candidates and selection diagnostics.
- [x] Add `VulkanRuntimePhysicalDeviceSelectionProbeResult` and `probe_runtime_physical_device_selection`.
- [x] Add a vector overload for `select_physical_device` so runtime probes can select from generated candidates.
- [x] Keep selection data first-party and avoid exposing `VkPhysicalDevice` handles.
- [x] Update RHI docs, architecture docs, agent manifest, and Codex/Claude rendering skills.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 43: Vulkan Persistent Runtime Instance Owner

**Files:**
- Modify: `engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_backend.hpp`
- Modify: `engine/rhi/vulkan/src/vulkan_backend.cpp`
- Modify: `tests/unit/backend_scaffold_tests.cpp`
- Modify: `docs/rhi.md`
- Modify: `docs/architecture.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/rendering-change/SKILL.md`
- Modify: `.claude/skills/gameengine-rendering/SKILL.md`

- [x] Write failing tests for a move-only persistent Vulkan runtime instance owner without exposing native handles.
- [x] Add `VulkanRuntimeInstance`, `VulkanRuntimeInstanceCreateResult`, and `create_runtime_instance`.
- [x] Create a persistent private native instance through `vkCreateInstance` after transient command probe readiness.
- [x] Destroy the instance through `vkDestroyInstance` and free the runtime library in `reset`/destructor without exposing handles.
- [x] Update RHI docs, architecture docs, agent manifest, and Codex/Claude rendering skills.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 44: Vulkan Runtime Logical Device Owner

**Files:**
- Modify: `engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_backend.hpp`
- Modify: `engine/rhi/vulkan/src/vulkan_backend.cpp`
- Modify: `tests/unit/backend_scaffold_tests.cpp`
- Modify: `docs/rhi.md`
- Modify: `docs/architecture.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/rendering-change/SKILL.md`
- Modify: `.claude/skills/gameengine-rendering/SKILL.md`

- [x] Write failing tests for a move-only persistent Vulkan runtime logical device owner without exposing native handles.
- [x] Add `VulkanRuntimeDevice`, `VulkanRuntimeDeviceCreateResult`, `vulkan_device_command_requests`, and `create_runtime_device`.
- [x] Build runtime logical-device plans from selected physical-device snapshots and device-extension lists.
- [x] Create private logical devices through `vkCreateDevice`, resolve device-scope commands through `vkGetDeviceProcAddr`, and fetch graphics/present queues through `vkGetDeviceQueue`.
- [x] Wait idle when possible and destroy `VkDevice`, `VkInstance`, and runtime library state through PIMPL-owned reset/destructor paths.
- [x] Update RHI docs, architecture docs, agent manifest, and Codex/Claude rendering skills.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 45: SDL3 Display and DPI Contract

**Files:**
- Modify: `engine/platform/include/mirakana/platform/window.hpp`
- Modify: `engine/platform/src/window.cpp`
- Modify: `engine/platform/sdl3/include/mirakana/platform/sdl3/sdl_window.hpp`
- Modify: `engine/platform/sdl3/src/sdl_window.cpp`
- Modify: `tests/unit/core_tests.cpp`
- Modify: `tests/unit/sdl3_platform_tests.cpp`
- Modify: `docs/architecture.md`
- Modify: `docs/roadmap.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`

- [x] Write failing tests for first-party display info and window display state validation.
- [x] Add `DisplayInfo`, `DisplayRect`, and `WindowDisplayState` without exposing native display or window handles.
- [x] Write failing SDL3 tests for display enumeration, content scale, pixel density, and safe-area reporting.
- [x] Map SDL3 display/window APIs into first-party display contracts with deterministic fallbacks.
- [x] Update AI-facing docs and game-development skills so agents use these contracts instead of native handles.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 46: Platform Clipboard Contract

**Files:**
- Create: `engine/platform/include/mirakana/platform/clipboard.hpp`
- Create: `engine/platform/src/clipboard.cpp`
- Create: `engine/platform/sdl3/include/mirakana/platform/sdl3/sdl_clipboard.hpp`
- Create: `engine/platform/sdl3/src/sdl_clipboard.cpp`
- Modify: `engine/platform/CMakeLists.txt`
- Modify: `engine/platform/sdl3/CMakeLists.txt`
- Modify: `tests/unit/core_tests.cpp`
- Modify: `tests/unit/sdl3_platform_tests.cpp`
- Modify: `docs/architecture.md`
- Modify: `docs/roadmap.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`

- [x] Write failing tests for deterministic first-party text clipboard behavior.
- [x] Add `IClipboard` and `MemoryClipboard` to `MK_platform`.
- [x] Write failing SDL3 tests for real text clipboard read/write/clear behavior.
- [x] Add `SdlClipboard` backed by SDL3 clipboard APIs.
- [x] Update AI-facing docs and game-development skills so agents use first-party clipboard contracts.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 47: Platform Cursor Contract

**Files:**
- Create: `engine/platform/include/mirakana/platform/cursor.hpp`
- Create: `engine/platform/src/cursor.cpp`
- Create: `engine/platform/sdl3/include/mirakana/platform/sdl3/sdl_cursor.hpp`
- Create: `engine/platform/sdl3/src/sdl_cursor.cpp`
- Modify: `engine/platform/CMakeLists.txt`
- Modify: `engine/platform/sdl3/CMakeLists.txt`
- Modify: `tests/unit/core_tests.cpp`
- Modify: `tests/unit/sdl3_platform_tests.cpp`
- Modify: `docs/architecture.md`
- Modify: `docs/roadmap.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`

- [x] Write failing tests for deterministic cursor normal, hidden, confined, and relative modes.
- [x] Add `CursorMode`, `CursorState`, `ICursor`, and `MemoryCursor` to `MK_platform`.
- [x] Write failing SDL3 tests for cursor mode mapping.
- [x] Add `SdlCursor` backed by SDL3 cursor visibility, window mouse grab, and relative mouse mode APIs.
- [x] Update AI-facing docs and game-development skills so agents use first-party cursor contracts.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 48: Platform Lifecycle Contract

**Files:**
- Create: `engine/platform/include/mirakana/platform/lifecycle.hpp`
- Create: `engine/platform/src/lifecycle.cpp`
- Modify: `engine/platform/CMakeLists.txt`
- Modify: `engine/platform/sdl3/include/mirakana/platform/sdl3/sdl_window.hpp`
- Modify: `engine/platform/sdl3/src/sdl_window.cpp`
- Modify: `tests/unit/core_tests.cpp`
- Modify: `tests/unit/sdl3_platform_tests.cpp`
- Modify: `docs/architecture.md`
- Modify: `docs/roadmap.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`

- [x] Write failing tests for deterministic quit, termination, low-memory, background, and foreground lifecycle events.
- [x] Add `LifecycleEventKind`, `LifecycleEvent`, `LifecycleState`, and `VirtualLifecycle` to `MK_platform`.
- [x] Write failing SDL3 tests for application lifecycle event mapping.
- [x] Map SDL3 quit, termination, low-memory, background, and foreground events into `VirtualLifecycle`.
- [x] Update AI-facing docs and game-development skills so agents use first-party lifecycle contracts.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 49: Display Monitor Selection Policy

**Files:**
- Modify: `engine/platform/include/mirakana/platform/window.hpp`
- Modify: `engine/platform/src/window.cpp`
- Modify: `engine/platform/sdl3/include/mirakana/platform/sdl3/sdl_window.hpp`
- Modify: `engine/platform/sdl3/src/sdl_window.cpp`
- Modify: `tests/unit/core_tests.cpp`
- Modify: `tests/unit/sdl3_platform_tests.cpp`
- Modify: `docs/architecture.md`
- Modify: `docs/roadmap.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`

- [x] Write failing tests for deterministic primary, specific, highest-scale, and largest-usable-area display selection.
- [x] Add `DisplaySelectionPolicy`, `DisplaySelectionRequest`, and `select_display` to `MK_platform`.
- [x] Write failing SDL3 tests for selection through real display enumeration.
- [x] Add `sdl3_select_display` to map SDL display enumeration through the first-party selection policy.
- [x] Update AI-facing docs and game-development skills so agents use first-party monitor selection.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 50: Window Placement Policy

**Files:**
- Modify: `engine/platform/include/mirakana/platform/window.hpp`
- Modify: `engine/platform/src/window.cpp`
- Modify: `engine/platform/sdl3/include/mirakana/platform/sdl3/sdl_window.hpp`
- Modify: `engine/platform/sdl3/src/sdl_window.cpp`
- Modify: `tests/unit/core_tests.cpp`
- Modify: `tests/unit/sdl3_platform_tests.cpp`
- Modify: `docs/architecture.md`
- Modify: `docs/roadmap.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`

- [x] Write failing tests for centered, top-left, and absolute window placement planning.
- [x] Add `WindowPosition`, `WindowPlacementPolicy`, `WindowPlacementRequest`, `WindowPlacement`, and `plan_window_placement`.
- [x] Extend `IWindow` and `HeadlessWindow` with `position`, `move`, and `apply_placement`.
- [x] Write failing SDL3 tests for window move and placement application.
- [x] Map SDL3 window position and size operations through first-party placement contracts.
- [x] Update AI-facing docs and game-development skills so agents use first-party window placement.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 51: Native File Dialog Contract

**Files:**
- Create: `engine/platform/include/mirakana/platform/file_dialog.hpp`
- Create: `engine/platform/src/file_dialog.cpp`
- Create: `engine/platform/sdl3/include/mirakana/platform/sdl3/sdl_file_dialog.hpp`
- Create: `engine/platform/sdl3/src/sdl_file_dialog.cpp`
- Modify: `engine/platform/CMakeLists.txt`
- Modify: `engine/platform/sdl3/CMakeLists.txt`
- Modify: `tests/unit/core_tests.cpp`
- Modify: `tests/unit/sdl3_platform_tests.cpp`
- Modify: `docs/architecture.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/testing.md`
- Modify: `docs/specs/2026-04-25-core-first-mvp-design.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`

- [x] Write failing tests for first-party file dialog request validation, scripted memory dialogs, and async result polling.
- [x] Add `FileDialogRequest`, `IFileDialogService`, `FileDialogResultQueue`, and `MemoryFileDialogService`.
- [x] Write failing SDL3 tests for dialog kind/filter mapping and async callback result translation.
- [x] Map SDL3 asynchronous native file dialog callbacks into first-party file dialog results without exposing native handles.
- [x] Update AI-facing docs and game-development skills so agents use asynchronous first-party file dialog contracts.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 52: Toolchain Dependency Policy Hardening

**Files:**
- Modify: `vcpkg.json`
- Create: `tools/check-dependency-policy.ps1`
- Modify: `tools/validate.ps1`
- Modify: `package.json`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `docs/dependencies.md`
- Modify: `docs/specs/2026-04-26-toolchain-dependency-policy-design.md`
- Modify: `README.md`
- Modify: `THIRD_PARTY_NOTICES.md`
- Modify: `AGENTS.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/cmake-build-system/SKILL.md`
- Modify: `.agents/skills/license-audit/SKILL.md`
- Modify: `.claude/skills/gameengine-feature/SKILL.md`
- Modify: `.claude/skills/gameengine-license-audit/SKILL.md`

- [x] Confirm current official vcpkg checkout baseline and selected SDL3/Dear ImGui versions.
- [x] Pin `vcpkg.json` with the official vcpkg `builtin-baseline`.
- [x] Add a dependency policy check for schema, baseline, default dependency isolation, GUI feature shape, notices, docs, and CMake manifest-feature use.
- [x] Add `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1` and include the policy check in `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Update docs and third-party notices to record the pinned dependency policy.
- [x] Update AI-facing instructions and skills so Codex/Claude preserve dependency reproducibility.

### Task 53: C++23 Migration Evaluation Harness

**Files:**
- Modify: `CMakeLists.txt`
- Modify: `CMakePresets.json`
- Create: `tools/check-cpp-standard-policy.ps1`
- Create: `tools/evaluate-cpp23.ps1`
- Modify: `tools/validate.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `package.json`
- Create: `docs/cpp-standard.md`
- Modify: `docs/cpp-style.md`
- Modify: `docs/workflows.md`
- Modify: `README.md`
- Modify: `AGENTS.md`
- Modify: `.agents/skills/cmake-build-system/SKILL.md`
- Modify: `.claude/rules/cpp-engine.md`
- Modify: `.claude/skills/gameengine-feature/SKILL.md`
- Modify: `engine/agent/manifest.json`

- [x] Add a failing policy check for configurable C++ standard support and C++23 evaluation surfacing.
- [x] Add `MK_CXX_STANDARD=23` evaluation presets before promoting C++23 to the baseline.
- [x] Add `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/evaluate-cpp23.ps1` for configure/build/test in C++23 mode.
- [x] Add C++23 Release package evaluation preset and `tools/evaluate-cpp23.ps1 -Release`.
- [x] Add optional GUI C++23 evaluation through `tools/evaluate-cpp23.ps1 -Gui`.
- [x] Document local C++23 evaluation results, migration recommendation, and gates.
- [x] Update AI-facing instructions with temporary C++23 evaluation gates before baseline promotion.

### Task 54: C++23 Baseline Migration

**Goal:** Promote C++23 from evaluation mode to the required GameEngine language baseline.

**Context:** Local Windows Debug, Release package, and optional SDL3/Dear ImGui C++23 verification passed on 2026-04-26.

**Done when:**

- [x] Set `MK_CXX_STANDARD=23` as the only accepted CMake standard value.
- [x] Update engine and game agent manifests to declare `C++23`.
- [x] Update game manifest schema and new-game scaffold output to require `C++23`.
- [x] Update Codex and Claude Code rules/skills to treat C++23 as required.
- [x] Update docs and validation checks for the C++23 baseline.

### Task 55: MSVC C++23 Preview And Module Policy

**Goal:** Align the C++23 baseline with the current official MSVC/CMake support surface.

**Context:** CMake 3.31 maps MSVC `cxx_std_23` to `/std:c++latest`, while MSVC exposes C++23 preview as `/std:c++23preview`. CMake module scanning is available, but CMake-managed `import std;` is generator/toolchain gated.

**Done when:**

- [x] Add centralized `MK_MSVC_CXX23_STANDARD_OPTION` policy and configure presets.
- [x] Override MSVC C++23 compile mode to `/std:c++23preview`.
- [x] Enable CMake C++ module scanning policy through `MK_ENABLE_CXX_MODULE_SCANNING`.
- [x] Add `MK_ENABLE_IMPORT_STD` gating on `CMAKE_CXX_COMPILER_IMPORT_STD`.
- [x] Update Codex/Claude guidance and C++ docs for modules, `import std;`, and C++23-only feature adoption.

### Task 56: Project-Wide Plan Review And Next-Phase Roadmap

**Goal:** Close the core-first MVP plan as historical implementation evidence and move production engine work to a dedicated next-phase roadmap.

**Context:** The MVP plan has grown beyond the initial core-first target and now records completed C++23, dependency, AI, editor, RHI, asset, scene, physics, audio, and animation foundations.

**Done when:**

- [x] Create `docs/superpowers/plans/2026-04-26-engine-excellence-roadmap.md`.
- [x] Update `docs/roadmap.md` so completed governance work is not listed as future work.
- [x] Update `docs/workflows.md` so CI documentation matches `.github/workflows/validate.yml`.
- [x] Update `engine/agent/manifest.json` next runtime targets to match the active roadmap.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
