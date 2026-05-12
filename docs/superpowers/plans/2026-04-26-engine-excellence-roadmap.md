# Engine Excellence Roadmap Implementation Plan (2026-04-26)

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` for independent implementation slices or `superpowers:executing-plans` for sequential execution. Use `rendering-change` for RHI, shader, GPU, material, or viewport work; `cmake-build-system` for CMake, presets, package, module, sanitizer, or CI work; `editor-change` for editor and `MK_editor_core` work; `license-audit` before adding any dependency or asset; `gameengine-agent-integration` whenever API, manifest, skills, rules, subagents, or generated-game workflows change.

**Goal:** Turn the current C++23 core-first MVP into a production-grade desktop/mobile game engine that supports real 2D/3D rendering, editor workflows, asset import/cooking, gameplay systems, mobile packaging, release packaging, and AI-driven game creation through Codex and Claude Code.

**Architecture:** Preserve strict dependency direction. `engine/core` stays independent from OS, GPU, asset format, and editor code. Platform code lives behind first-party contracts in `engine/platform`. Graphics API implementations stay behind `engine/rhi` and renderer-facing abstractions in `engine/renderer`. `editor/core` remains GUI-independent. Dear ImGui remains the immediate-mode developer-tool shell, while production runtime UI and scalable editor surfaces now start from first-party retained-mode `MK_ui` and `MK_editor_ui` contracts rather than Dear ImGui panels. UI contracts are first-party, but mature low-level UI implementation details such as font rasterization, complex text shaping, IME, accessibility bridges, image decoding, and platform integration should live behind official-SDK or audited-dependency adapters instead of being hand-rolled by default. Native OS/window/GPU/tool handles remain backend-private through PIMPL or first-party opaque handles. The engine-facing AI contract remains `engine/agent/manifest.json`, with Codex and Claude Code guidance kept behaviorally equivalent.

**Tech Stack:** C++23, CMake presets, CTest, CPack, PowerShell 7 `tools/*.ps1` wrappers, vcpkg manifest features with a pinned `builtin-baseline`, MSVC `/std:c++23preview` through `MK_MSVC_CXX23_STANDARD_OPTION` until official stable `/std:c++23` is available, CMake `FILE_SET CXX_MODULES` and gated `CXX_MODULE_STD`, Direct3D 12, Vulkan 1.3 with dynamic rendering and synchronization2, Metal, SDL3, Dear ImGui for current developer tooling, first-party retained-mode `MK_ui`/`MK_editor_ui` headless contracts for production UI models and editor UI models, future adapter-only official SDK or audited dependencies for text/font/IME/accessibility/image-decoding/concrete-renderer needs, Android GameActivity/NDK, GitHub Actions.

---

## Plan Status

- This file is the active strategic roadmap and wave ledger.
- The plan registry is `docs/superpowers/plans/README.md`; update that registry whenever a new focused implementation plan is created or completed.
- Most host-independent roadmap waves are complete and recorded below as implementation evidence.
- The only unchecked item in this plan is host-gated Apple/iOS/Metal validation and backend-neutral Metal visible presentation. Do not mark it complete without a macOS/Xcode local or CI validation result.
- New host-independent production work should be split into a new dated focused plan instead of being appended to this roadmap.

---

## Audit Baseline

### Implemented and usable now

- C++23 baseline policy, MSVC C++23 preview override, module-scanning policy, and validation checks.
- Target-based CMake structure, CTest, package script, dependency policy, JSON contract checks, and GitHub Actions validation.
- Core, math, platform contracts, SDL3 optional desktop backend, process/filesystem/file-dialog/clipboard/cursor/lifecycle/input/mobile contracts.
- Renderer and RHI contracts with `NullRhiDevice`, `RhiFrameRenderer`, frame-between graphics pipeline replacement, `RhiViewportSurface`, shader-readable display frames, readback fallback frames, transient leases, swapchain semantics, fence semantics, shared texture export intent, and backend-neutral fence/shared-texture stats.
- D3D12 native backend scaffold with private device/resource/command-list/fence/swapchain/descriptor/root-signature/shader/PSO ownership and initial `IRhiDevice` bridge.
- Vulkan SDK-independent runtime probing, instance/device ownership, physical-device snapshots, surface support, command resolution planning, Vulkan 1.3/swapchain/dynamic-rendering readiness checks.
- Metal SDK-independent backend scaffold.
- Scene, prefab, camera/light/mesh/sprite renderer components, scene serialization, scene-to-renderer bridge, material/material-instance foundations.
- Asset registry, dependency graph, hot reload snapshots, first-party deterministic texture/mesh/audio/material source documents, cooked artifact planning/execution, cooked package index serialization with content hashes, deterministic recook decisions, explicit material binding metadata, shader tool discovery, backend-specific shader toolchain readiness diagnostics including DXC SPIR-V CodeGen capability, shader compile action/cache/provenance.
- Device-independent audio mixer with bus/voice mix planning, clip metadata, streaming buffer scheduling, device-format conversion intent, underrun diagnostics, and deterministic interleaved float PCM generation; deterministic 2D/3D physics foundation with 2D/3D broadphase, 2D circle contacts, 3D sphere/capsule contacts, layers/masks, iterative solver configuration with correction slop/percent, stacked-body coverage, collision-bounds raycasts, and replay coverage; deterministic grid navigation through `MK_navigation`; plus keyframe/state-machine/blend-tree/blend-graph/layered/curve-binding/timeline-event-track/retargeting animation foundations.
- Headless first-party `MK_ui` retained-mode contracts with deterministic document hierarchy, style/layout descriptors, row/column/stack layout solving with row/column gaps, text metadata, focus/navigation/modal state, transition state, one-way text binding, command invocation, renderer submission payload construction with `RendererBox` and `RendererTextRun` records, accessibility node payload construction with localization key, enabled, and focusable metadata, plus `MK_runtime` localization catalog support for deterministic key-to-text lookup, and adapter boundaries for text shaping, bidirectional text, line breaking, font rasterization, glyph atlases, IME, accessibility, image decoding, concrete renderer adapters, and platform integration.
- Optional SDL3 + Dear ImGui editor shell with project IO, command palette, asset browser/import queue, shader compile status, D3D12 DXIL and Vulkan SPIR-V viewport shader readiness display, viewport controls, Inspector editing, undo/redo, D3D12 shared-texture viewport display when SDL is using D3D12, and CPU-readback fallback display. This is the current developer-tool UI surface, not the final production runtime UI system.
- GUI-independent `MK_editor_ui` model helpers in `editor/core` for inspector property rows, asset list rows, command palette entries, diagnostics panels, timeline controls/event rows, deterministic editor UI model serialization, plus asset-pipeline panel aggregation for progress, dependency rows, thumbnail requests, material previews, and import diagnostics.
- AI-facing `AGENTS.md`, `CLAUDE.md`, repository skills, Claude rules, Codex/Claude subagent definitions, game manifests, and `engine/agent/manifest.json`.

### Highest-priority gaps

- D3D12 non-black render-target clear, renderer viewport clear-color, first-triangle texture readback proof, `IRhiDevice` texture state validation, stale cross-queue texture-state submission rejection, descriptor/root-signature/PSO validation failures, backend-neutral swapchain render/present/resize sequencing, explicit acquire/release accounting, single-pending-frame swapchain throttling, backend-neutral fence lifecycle stats, and D3D12 shared-texture editor display interop now exist; Vulkan and Metal still need equivalent production viewport paths.
- Vulkan now has native command-pool/primary-command-buffer, runtime buffer create planning and buffer/memory ownership plus first-party upload/readback-domain buffer mapping, runtime texture create planning and image/memory ownership including private depth-aspect image views, shader-module, descriptor set layout/pool/set allocation, buffer descriptor updates, and descriptor binding, pipeline-layout, graphics-pipeline with public depth-state mapping, a backend-neutral `IRhiDevice` bridge for buffer/texture creation, upload-buffer writes, transient leases, shader modules, descriptor set layout/allocation/update, pipeline layout, graphics pipeline creation, surface-backed swapchain creation/resize, explicit swapchain frame acquire/release, shader-free swapchain clear render passes, standalone texture transitions including `depth_write`, standalone texture render-target clears and draw recording with optional depth attachments plus buffer/buffer-texture copy commands, graphics command-list close/submit, queued present, copy-only submit, immediate fence stats/waits, and readback-buffer CPU reads, Win32 surface/swapchain/image-view, frame sync/acquire ownership, queue-present result mapping, native dynamic rendering clear/readback smoke coverage, native dynamic rendering draw recording with color/depth attachments, native synchronization2 swapchain/texture barrier plus command-buffer submission recording, native readback buffer/copy/mapping ownership, DXC SPIR-V CodeGen readiness diagnostics, `IRhiDevice` mapping readiness gates for descriptor binding plus visible clear/readback, visible draw/readback, visible texture-sampling/readback, and visible depth/readback proof, and environment-gated real-SPIR-V visible draw/readback, raw texture-sampling, depth-ordered draw/readback, sampled-depth readback, and `RhiFrameRenderer` runtime material texture-sampling coverage when local Vulkan runtime and shader artifacts are configured.
- Metal now has SDK-independent non-empty `metallib` artifact validation, runtime/default-device/command-queue readiness planning, texture resource synchronization planning, platform availability diagnostics, Apple-only Objective-C++ CMake gating, native default-device/command-queue/command-buffer/render-encoder/texture-target/`CAMetalLayer` drawable/present/CPU-readback ownership behind private PIMPL on Apple hosts, and macOS CI definition; it still lacks shader library creation and visible-frame integration.
- External production importers for PNG, glTF, and common audio formats are integrated as optional `MK_tools` adapters behind the `asset-importers` feature using `libspng`, `fastgltf`, and `miniaudio`. The default build remains dependency-light, runtime/game code remains cooked-artifact-only, first-party texture/mesh/audio source fixtures now preserve optional byte payloads into cooked artifacts, PNG adapters now emit decoded RGBA8 pixel payloads, common-audio adapters now emit decoded PCM sample payloads, and glTF adapters now cook triangle `POSITION` payloads plus `uint32` normalized indices into deterministic mesh byte payloads for embedded/loaded buffers. glTF editor UX remains follow-up work.
- Material system is usable as data and now has explicit backend-neutral binding metadata for descriptor/pipeline layout derivation, including sampled texture and sampler rows plus a fixed material factor uniform size. `MK_runtime_rhi` creates backend-neutral texture resources, mesh vertex/index buffers, material factor uniform uploads backed by 256-byte uniform allocations, and owner-device-validated sampler-aware material descriptor bindings from typed runtime payloads. Initial renderer-visible mesh/material command binding, vertex input metadata, RHI vertex/index buffer binding, indexed draw recording, shader reflection metadata, scene GPU binding resolution through `SceneGpuBindingPalette`, material preview uniform/binding rows including emissive factor exposure, selected cooked material preview status/artifact/diagnostic/texture-dependency rows, GUI-independent GPU binding preview planning from typed runtime payloads, native D3D12 sampler descriptor table/root-signature mapping, native Vulkan sampled-image/sampler descriptor writes, D3D12 visible texture-sampling proof, environment-gated Vulkan texture-sampling proof, backend-neutral `RhiViewportSurface` runtime material descriptor binding proof, backend-neutral `RhiFrameRenderer` runtime material descriptor binding coverage, D3D12 visible `RhiFrameRenderer` runtime material texture sampling, environment-gated Vulkan `RhiFrameRenderer` runtime material texture sampling, material-preview-specific factor-only and base-color-textured shader artifact request/readiness tracking for D3D12 DXIL and Vulkan SPIR-V, editor selected-material GPU preview bytecode selection from those material preview artifacts, initial editor selected-material GPU binding preview display through runtime texture uploads and a small RHI viewport surface, runtime package-to-`SceneMaterialPalette` loading through `load_runtime_scene_render_data`, non-throwing runtime scene render packet instantiation through `instantiate_runtime_scene_render_data`, runtime package/payload/scene/session diagnostic reports, and sample cooked scene/material usage in `sample_ui_audio_assets` now exist; broader material/shader workflows remain follow-up work. `MK_runtime` now also owns deterministic save data, settings, localization catalog, keyboard input action mapping documents, serialized scene payload access, package-level scene material dependency resolution, and load-time diagnostic report helpers; broader input devices, save/settings layering, and runtime profiling/telemetry diagnostics remain follow-up work.
- Audio now has device-independent streaming buffer scheduling, device-format conversion intent, underrun diagnostics, deterministic interleaved float PCM generation with selectable nearest/linear resampling, an optional SDL3 desktop output adapter for default playback streams, and an Android AAudio output adapter validated through device/emulator package smoke; remaining audio work is streaming decode integration and richer conversion quality gates.
- Physics now has deterministic 2D/3D sweep-and-prune broadphase, 2D circle contacts, 3D sphere/capsule contacts, collision layers/masks, iterative solver configuration with correction slop/percent, stacked-body coverage, replay coverage, and an advanced 3D backend evaluation that keeps third-party physics out of the current wave while selecting Jolt as the preferred future optional adapter candidate.
- Animation now has deterministic blend tree samples, 1D blend graph samples, layered samples, curve binding samples, state machines, authored timeline event tracks, timeline playback, retargeting policy evaluation, GUI-independent editor timeline models, and a Dear ImGui timeline adapter.
- Mobile packaging now has SDK-independent lifecycle/touch/orientation/safe-area/storage/permission contracts, Android Gradle/NDK/GameActivity CMake/Prefab templates, host-validated Android Debug and Release APK builds, Android release-signing verification with a non-repository upload key, Android emulator install/launch smoke coverage, an Android native Vulkan surface creation bridge, an Android AAudio output adapter, Android game manifest/resource asset packaging, Android release-signing environment contracts, iOS CMake/Xcode UIKit/Metal bundle templates, iOS Metal-backed view setup, iOS sandbox save/cache/shared storage root mapping, iOS game manifest/resource bundling, iOS signing configuration inputs, iOS Simulator smoke scripting plus macOS GitHub Actions lane definition, game-manifest validation, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-mobile-packaging.ps1` diagnostics; iOS IPA/simulator execution remains gated on Apple toolchains.
- Hot reload has platform file watcher backend selection, native backend family reporting, deterministic polling file events, Windows native `ReadDirectoryChangesW`, Linux native `inotify`, and macOS native `FSEvents` watchers, host-unavailable native watcher diagnostics, debounce-based recook request scheduling, dependency invalidation, failed-reload rollback state, runtime asset replacement staging with safe-point commits, editor/core plus GUI recook/apply diagnostic state, recook import plan extraction, editor and runtime recook execution through the asset import tool path, shader artifact hot reload planning through provenance, and backend pipeline recreation boundaries.
- Production UI architecture now has a headless first-party `MK_ui` contract baseline, deterministic row/column/stack layout solving with row/column gaps, renderer submission payloads with `RendererBox` and `RendererTextRun` records, accessibility node payloads with localization key/enabled/focusable metadata, a first-party `MK_ui_renderer` adapter that submits styled UI boxes through `MK_renderer`, `MK_runtime` localization catalogs for key-to-text lookup, and `MK_editor_ui` editor model helpers, with the asset-pipeline editor model now adapted into the current Dear ImGui shell. Remaining UI work is deeper renderer/text/font/accessibility adapters, richer layout models, deeper accessibility metadata, broader localization integration, broader Dear ImGui editor-shell adapters for retained models, and concrete official-SDK or audited-dependency adapters for text shaping, font rasterization, IME, OS accessibility, image decoding, and UI rendering.
- Release packaging now has a baseline installed SDK layout, installed-consumer validation, package metadata, sample/source package content, and CI artifact publishing; editor package depth, symbols, and broader release artifact signing/publishing remain follow-up work.
- Static analysis now has a documented clang-tidy profile and coverage scripts, but local MSVC generator runs still report compile database availability as a diagnostic blocker; strict tidy/coverage enforcement and expanded compiler/platform matrix coverage remain future work.

### Current Review Snapshot

- Completed waves: Wave 0 governance, Wave 1 quality gates, Wave 2 D3D12 visible backend/editor viewport, Wave 5 asset import/cook/material binding baseline, Wave 6 runtime audio/physics/animation production pass, Wave 7 hot reload/live iteration contracts, Wave 8 headless `MK_ui`/`MK_ui_renderer`/`MK_editor_ui` UI contracts, Wave 10 SDK/samples/release packaging baseline, and Wave 11 AI-driven game creation excellence.
- Active priority: Wave 4 Metal drawable/present ownership and macOS CI definition are in place; remaining Metal work is shader library creation, backend-neutral visible integration, and hosted Apple runner validation.
- Secondary priority: continue iOS simulator/device validation, streaming decode integration, glTF importer/editor UX, Vulkan editor material-preview display promotion, deeper runtime diagnostics/profiling, and gameplay-facing runtime scene workflows while keeping Vulkan/Metal/importer work platform- and dependency-gated.
- Dependency-gated priorities: additional Wave 5 importer depth and UI/text/font/image adapters must not start without `license-audit`, dependency records, `vcpkg.json`, and `THIRD_PARTY_NOTICES.md` updates.
- Platform-gated priorities: Wave 4 Metal and deeper Wave 9 mobile package builds need Apple/Android toolchain availability and explicit local or CI validation paths.
- Guidance synchronization remains continuous: whenever public APIs, game-generation guidance, manifests, skills, rules, subagents, or validation recipes change, update Wave 11 surfaces in the same task.

## Official Guidance Locks

- Use CMake targets and file sets. Project modules must be added through `target_sources(FILE_SET CXX_MODULES)`, with module scanning controlled through CMake properties and `import std;` enabled only when CMake reports support through `CMAKE_CXX_COMPILER_IMPORT_STD`.
- Use vcpkg manifest mode for optional C++ dependencies because it isolates direct project dependencies in `vcpkg.json` and supports reproducible resolution through the pinned baseline.
- Use MSVC `/std:c++23preview` for strict C++23-only mode until official stable `/std:c++23` is available; do not use `/std:c++latest` unless a task explicitly accepts future-working-draft exposure.
- Use D3D12 command queues/lists, explicit resource barriers, fences, descriptor ownership, and debug-layer validation as first-class design objects.
- Use Vulkan 1.3 as the active Vulkan target, with swapchain support, dynamic rendering, synchronization2, explicit image layout transitions, timeline/semaphore/fence strategy, and SPIR-V artifact validation.
- Compile HLSL for Vulkan through DXC using executable-plus-argument vectors with `-spirv` and `-fspv-target-env=vulkan1.3`; require a DXC build with SPIR-V CodeGen enabled before claiming Vulkan shader readiness, and do not rely on shell strings or unspecified SPIR-V target environments.
- Use Metal device/command-queue/command-buffer/render-pass/resource synchronization concepts directly behind `MK_rhi_metal`, without leaking Objective-C/Metal handles to public engine APIs.
- Use Android GameActivity and Prefab/CMake integration for Android C++ games rather than `NativeActivity` as the engine default mobile entry path.
- Treat Dear ImGui as the current developer-facing shell for diagnostics and early editor tooling. Do not use it as the production runtime UI foundation; model long-term UI architecture after the separation used by Unreal's Slate/UMG/Common UI and Unity's UI Toolkit/uGUI/IMGUI families.
- Own the UI contract, not every low-level UI implementation detail. Do not hand-roll mature text shaping, font rasterization, IME, accessibility bridge, image decoding, or platform integration systems unless an accepted architecture decision proves that official SDK or audited dependency adapters are worse.
- Evaluate Qt, NoesisGUI, Slint, RmlUi, or any other UI middleware only as optional adapters after license audit and dependency policy updates; do not replace or leak through the first-party `MK_ui` abstraction.
- Keep all third-party code, assets, shaders, fonts, and generated material under explicit license tracking in `docs/legal-and-licensing.md`, `docs/dependencies.md`, `vcpkg.json`, and `THIRD_PARTY_NOTICES.md`.

## Execution Waves

### Wave 0: Planning and Governance Synchronization

**Files:**
- Create: `docs/superpowers/plans/2026-04-26-engine-excellence-roadmap.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/workflows.md`
- Modify: `docs/superpowers/plans/2026-04-25-core-first-mvp.md`
- Modify: `engine/agent/manifest.json`

**Steps:**
- [x] Record the project-wide implementation audit and highest-priority gaps.
- [x] Replace stale roadmap priorities with execution waves.
- [x] Sync workflow docs with the actual GitHub Actions validation matrix.
- [x] Update AI manifest next targets so Codex/GPT/Claude workers see the same runtime order.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

**Done when:** The repository has one current next-phase plan, stale roadmap items no longer list completed policy work as future work, AI targets point to real runtime gaps, and validation passes.

### Wave 1: Engineering Quality Gates

**Files:**
- Modify: `.github/workflows/validate.yml`
- Modify: `CMakePresets.json`
- Modify: `tools/validate.ps1`
- Modify: `tools/check-format.ps1`
- Create or modify: `tools/check-tidy.ps1`
- Create or modify: `tools/check-coverage.ps1`
- Modify: `docs/workflows.md`
- Modify: `docs/cpp-standard.md`

**Steps:**
- [x] Add a clang-tidy profile that starts with correctness, performance, modernize, bugprone, and readability checks appropriate for engine code.
- [x] Add a script that runs clang-tidy only when the active toolchain is available and reports an explicit local-tool blocker otherwise.
- [x] Add coverage collection for compiler/platform combinations where it is stable.
- [x] Add GitHub Actions artifact publishing for test logs, packages, and coverage output.
- [x] Keep C++23 checks tied to `MK_MSVC_CXX23_STANDARD_OPTION` instead of individual target flags.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

**Done when:** CI and local validation produce deterministic build/test/static-analysis outputs without broadening default third-party dependency scope.

### Wave 2: Direct3D 12 Visible Backend and Editor Viewport

**Files:**
- Modify: `engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp`
- Modify: `engine/rhi/d3d12/src/d3d12_backend.cpp`
- Modify: `engine/rhi/include/mirakana/rhi/rhi.hpp`
- Modify: `engine/renderer/include/mirakana/renderer/rhi_frame_renderer.hpp`
- Modify: `engine/renderer/include/mirakana/renderer/rhi_viewport_surface.hpp`
- Modify: `engine/renderer/src/rhi_frame_renderer.cpp`
- Modify: `engine/renderer/src/rhi_viewport_surface.cpp`
- Modify: `editor/src/main.cpp`
- Modify: `tests/unit/rhi_tests.cpp`
- Modify: `tests/unit/d3d12_rhi_tests.cpp`
- Modify: `tests/unit/renderer_rhi_tests.cpp`
- Modify: `docs/rhi.md`
- Modify: `engine/agent/manifest.json`

**Steps:**
- [x] Add failing tests for a non-black D3D12 clear through `IRhiDevice` with CPU readback assertions, then make them pass.
- [x] Add D3D12 `RhiViewportSurface` readback coverage proving renderer clear colors flow into real GPU output.
- [x] Add first-triangle frame CPU readback assertions through `IRhiDevice`.
- [x] Add failing tests for swapchain render/present/resize sequencing using first-party surface handles, then make them pass through NullRHI and D3D12.
- [x] Extend swapchain lifecycle tests to reject multiple pending frames for one swapchain before the prior frame is presented and submitted.
- [x] Extend swapchain lifecycle tests to explicit acquire/release sequencing and single-pending-frame throttling policy.
- [x] Harden D3D12 `IRhiDevice` texture state tracking for render target, shader read, copy source, copy destination, depth write, and present initial states before render/copy/transition commands.
- [x] Harden D3D12 swapchain/present state tracking for graphics-queue-only present, render-pass-complete-before-present, duplicate-present rejection, and resize-before-pending-present gating.
- [x] Harden D3D12 cross-queue hazards beyond the current back-buffer transition commands.
- [x] Add descriptor table, CBV/SRV/UAV, root signature, PSO, and shader bytecode validation failures before native calls.
- [x] Add backend-neutral RHI fence lifecycle stats for waits, wait failures, last submitted fence values, and last completed fence values across `NullRhiDevice` and D3D12.
- [x] Keep all native D3D12/DXGI handles private to backend/PIMPL types.
- [x] Replace the editor D3D12 CPU-readback display path with shared-texture interop behind an editor-private SDL texture bridge, while preserving CPU readback as the fallback.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

**Done when:** A D3D12 viewport can visibly clear/draw/present, the editor can use a low-latency D3D12 texture path on Windows, and tests prove backend-neutral contracts still work with `NullRhiDevice`.

### Wave 3: Vulkan Visible Backend

**Files:**
- Modify: `engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_backend.hpp`
- Modify: `engine/rhi/vulkan/src/vulkan_backend.cpp`
- Modify: `engine/rhi/include/mirakana/rhi/rhi.hpp`
- Modify: `engine/renderer/src/rhi_frame_renderer.cpp`
- Modify: `engine/renderer/src/rhi_viewport_surface.cpp`
- Modify: `tests/unit/backend_scaffold_tests.cpp`
- Modify: `tests/unit/rhi_tests.cpp`
- Modify: `tests/unit/renderer_rhi_tests.cpp`
- Modify: `docs/rhi.md`
- Modify: `engine/agent/manifest.json`

**Steps:**
- [x] Add command pool and command buffer ownership behind move-only private runtime objects.
- [x] Add swapchain, image view, surface format, present mode, image acquire, and resize planning.
- [x] Add native Win32 surface, swapchain, swapchain-image enumeration, and image-view ownership behind move-only private runtime objects.
- [x] Add native semaphore/fence frame sync ownership and first-party swapchain image acquire result mapping.
- [x] Add first-party queue-present result mapping without exposing `VkPresentInfoKHR` or `VkQueue`.
- [x] Add Vulkan 1.3 dynamic rendering path with explicit `vkCmdBeginRendering`/`vkCmdEndRendering` command resolution.
- [x] Add native dynamic rendering draw recording with private `VkRenderingInfo`, viewport/scissor setup, graphics pipeline binding, draw, and end-rendering.
- [x] Add synchronization2 barriers and frame submission ordering for acquire, render, readback, and present.
- [x] Add native synchronization2 swapchain image barrier recording and ended command-buffer submission through private `vkCmdPipelineBarrier2`/`vkQueueSubmit2` mirrors.
- [x] Add native readback buffer ownership, swapchain image-to-buffer copy recording, and CPU byte mapping through private Vulkan buffer/memory/copy mirrors.
- [x] Add native runtime buffer create planning and move-only buffer/memory ownership for first-party `BufferDesc` usage and memory-domain mapping without exposing `VkBuffer` or `VkDeviceMemory`.
- [x] Add native runtime texture create planning and move-only image/memory ownership for first-party `TextureDesc` usage mapping without exposing `VkImage` or `VkDeviceMemory`.
- [x] Add an initial Vulkan `IRhiDevice` resource bridge for backend-neutral buffer/texture creation and transient leases backed by private runtime buffer/image owners.
- [x] Add native descriptor set layout ownership, descriptor pool/set allocation, descriptor-aware pipeline layout creation, and command-buffer descriptor set binding through private Vulkan descriptor mirrors.
- [x] Extend the initial Vulkan `IRhiDevice` bridge to backend-neutral shader module, descriptor set layout/allocation, pipeline layout, and graphics pipeline creation backed by private runtime owners.
- [x] Extend the initial Vulkan `IRhiDevice` bridge to surface-backed swapchain creation/resize and explicit swapchain frame acquire/release backed by private swapchain and frame-sync owners.
- [x] Add a Windows runtime smoke path for dynamic-rendering clear, swapchain image readback, non-zero CPU byte verification, and present/resize-required handling when a Vulkan runtime is available.
- [x] Add SPIR-V shader artifact validation plus native shader module, pipeline layout, and graphics pipeline ownership before command recording.
- [x] Tighten the first-party `IRhiDevice` mapping readiness plan so descriptor binding, visible clear/readback proof, visible draw/readback proof, visible texture-sampling/readback proof, and visible depth/readback proof are required before Vulkan is advertised as RHI-ready.
- [x] Add editor/core and GUI readiness reporting for non-empty Vulkan vertex/fragment SPIR-V artifacts so future Vulkan viewport promotion has the same artifact gate as D3D12.
- [x] Lock engine-generated DXC Vulkan SPIR-V commands to `-spirv` plus `-fspv-target-env=vulkan1.3`.
- [x] Discover `spirv-val`, evaluate D3D12/Vulkan/Metal shader toolchain readiness, surface it through editor state/GUI, and add `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1` diagnostics.
- [x] Separate DXIL-capable DXC from DXC builds with SPIR-V CodeGen in shader toolchain readiness, GUI diagnostics, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`.
- [x] Allow editor shader tool executable settings to use reviewed absolute installed SDK/tool paths while keeping working directory, artifact root, and cache index paths project-relative.
- [x] Merge known installed Windows SDK `dxc.exe` and `VULKAN_SDK`/`VK_SDK_PATH` `spirv-val.exe` locations into GUI shader tool discovery so editor Project Settings can select real local SDK tools.
- [x] Add shell-free `spirv-val` validation command planning, process bridging, and artifact validation actions before wiring native Vulkan shader module promotion paths.
- [x] Cover shader compiler and `spirv-val` process runners with reviewed absolute executable override tests while keeping shell executable overrides rejected.
- [x] Add editor/core default viewport shader compile request planning that queues D3D12 DXIL and Vulkan SPIR-V artifacts from one HLSL source.
- [x] Add first-party Vulkan descriptor set updates for buffer descriptors through private `vkUpdateDescriptorSets` mirrors and the backend-neutral `IRhiDevice::update_descriptor_set` bridge.
- [x] Add first-party Vulkan standalone texture transitions, buffer copies, buffer-to-texture copies, texture-to-buffer copies, and copy-only command-list submission through private Vulkan command mirrors.
- [x] Add first-party Vulkan standalone texture render-target clear passes through private texture image views, dynamic-rendering clear recording, copy-to-readback proof, and backend-neutral `IRhiCommandList::begin_render_pass`.
- [x] Add first-party Vulkan standalone texture render-target draw recording through private texture image views, graphics pipeline binding, viewport/scissor setup, dynamic rendering, and backend-neutral `IRhiCommandList::draw`.
- [x] Complete Vulkan visible draw/readback proof through `IRhiDevice` using real SPIR-V shader artifacts and a Vulkan runtime (swapchain draw recording, standalone texture draw recording, clear/readback, graphics command-list close/submit, queued present, immediate fence stats/waits, readback-domain buffer reads, backend-neutral descriptor updates, standalone texture copy commands, backend-neutral resource/shader/descriptor/pipeline/swapchain creation, and an environment-gated real-SPIR-V draw/readback test exist).
- [x] Add visible draw readback tests where SPIR-V shader artifacts and a Vulkan runtime are available.
- [x] Add environment-gated Vulkan `RhiFrameRenderer` runtime material texture-sampling proof using real SPIR-V material shaders, runtime texture upload, sampler-aware material descriptor binding, renderer submission, and readback validation.
- [x] Run validation on both required lanes (`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed on Windows on 2026-04-27; WSL Ubuntu 24.04 + Clang 18 + Ninja Linux CMake/CTest passed on 2026-04-27 with C++ module scanning enabled).

**Done when:** Vulkan is a real `IRhiDevice` backend for visible clear/draw/readback/present without exposing Vulkan headers or native handles in public APIs.

### Wave 4: Metal Native Backend

**Files:**
- Modify: `engine/rhi/metal/include/mirakana/rhi/metal/metal_backend.hpp`
- Modify: `engine/rhi/metal/src/metal_backend.cpp`
- Modify: `engine/rhi/CMakeLists.txt`
- Modify: `CMakeLists.txt`
- Modify: `tests/unit/backend_scaffold_tests.cpp`
- Modify: `tests/unit/rhi_tests.cpp`
- Modify: `docs/rhi.md`
- Modify: `docs/workflows.md`
- Modify: `engine/agent/manifest.json`

**Steps:**
- [x] Add Apple-only CMake/Objective-C++ gating for native Metal implementation files.
- [x] Add native default-device and command-queue ownership behind PIMPL (SDK-independent readiness planning now gates runtime/default-device/command-queue availability; Apple-only Objective-C++ owns `MTLDevice`/`MTLCommandQueue` through private retained objects).
- [x] Add shader library validation for non-empty Metal library artifacts.
- [x] Add drawable/present ownership (`CAMetalLayer` drawable acquisition, drawable render-encoder creation, present scheduling, command buffer, render command encoder, texture target, CPU readback, and command-buffer submission ownership are implemented behind PIMPL; Apple runner execution remains host-gated).
- [x] Add resource synchronization tests and platform availability diagnostics.
- [x] Add macOS CI or documented local validation once hosted runner/toolchain support is enabled (GitHub Actions macOS Metal CMake lane added; hosted execution remains dependent on runner availability).

**Done when:** Metal can perform visible clear/draw/readback/present on Apple hosts through backend-neutral RHI contracts.

### Wave 5: Production Asset Import, Cook, and Material Binding

**Files:**
- Modify: `engine/assets/include/mirakana/assets/asset_import_pipeline.hpp`
- Modify: `engine/assets/src/asset_import_pipeline.cpp`
- Create or modify: `engine/assets/include/mirakana/assets/asset_package.hpp`
- Create or modify: `engine/assets/src/asset_package.cpp`
- Modify: `engine/assets/include/mirakana/assets/material.hpp`
- Modify: `engine/assets/src/material.cpp`
- Modify: `engine/tools/include/mirakana/tools/asset_import_tool.hpp`
- Modify: `engine/tools/src/asset_import_tool.cpp`
- Modify: `editor/core/include/mirakana/editor/asset_pipeline.hpp`
- Modify: `editor/core/src/asset_pipeline.cpp`
- Modify: `editor/src/main.cpp`
- Modify: `vcpkg.json`
- Modify: `docs/dependencies.md`
- Modify: `docs/legal-and-licensing.md`
- Modify: `THIRD_PARTY_NOTICES.md`
- Create: `docs/specs/2026-04-27-importer-dependency-audit.md`
- Modify: `tests/unit/tools_tests.cpp`
- Modify: `tests/unit/editor_core_tests.cpp`

**Steps:**
- [x] Run a license audit before selecting PNG, glTF, and audio decode libraries.
- [x] Add optional vcpkg manifest features for importer dependencies instead of default-build dependencies.
- [x] Add external source importers behind `MK_tools` so runtime/game code never parses external formats directly.
- [x] Add cooked package index generation, content hashes, dependency edges, and deterministic recook decisions.
- [x] Add shader reflection or explicit material binding metadata for backend-neutral descriptor/pipeline layout derivation.
- [x] Add GUI-independent editor/core models for import progress, thumbnail requests, dependency display, material preview, and import diagnostics.
- [x] Wire editor import progress, thumbnail requests, dependency display, material preview, and import diagnostics into the Dear ImGui editor shell.
- [x] Add GUI-independent GPU binding preview planning from cooked runtime material/texture payloads and wire an initial native-RHI selected-material binding preview display into the Dear ImGui editor shell.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` and importer feature-specific validation.

**Done when:** External art assets can be legally imported, cooked, referenced by materials/scenes, hot-reloaded safely, and consumed by render backends through cooked artifacts.

### Wave 6: Runtime Audio, Physics, and Animation Production Pass

**Files:**
- Modify: `engine/audio/include/mirakana/audio/audio_mixer.hpp`
- Modify: `engine/audio/src/audio_mixer.cpp`
- Create: `engine/audio/sdl3/include/mirakana/audio/sdl3/sdl_audio_device.hpp`
- Create: `engine/audio/sdl3/src/sdl_audio_device.cpp`
- Create: `engine/audio/sdl3/CMakeLists.txt`
- Modify: `engine/physics/include/mirakana/physics/physics2d.hpp`
- Modify: `engine/physics/include/mirakana/physics/physics3d.hpp`
- Modify: `engine/physics/src/physics2d.cpp`
- Modify: `engine/physics/src/physics3d.cpp`
- Create: `docs/specs/2026-04-27-physics-backend-evaluation.md`
- Modify: `engine/animation/include/mirakana/animation/state_machine.hpp`
- Modify: `engine/animation/include/mirakana/animation/timeline.hpp`
- Modify: `engine/animation/src/state_machine.cpp`
- Modify: `engine/animation/src/timeline.cpp`
- Modify: `editor/core/include/mirakana/editor/scene_edit.hpp`
- Modify: `editor/src/main.cpp`
- Modify: `tests/unit/core_tests.cpp`
- Create: `tests/unit/sdl3_audio_tests.cpp`
- Modify: `tests/unit/editor_core_tests.cpp`

**Steps:**
- [x] Add a real desktop audio device backend after deciding whether SDL3 audio is sufficient for the first production path.
- [x] Add streaming buffers, mixer scheduling, device format conversion, and underrun diagnostics.
- [x] Add 3D physics broadphase, sphere collision shape coverage, iterative solver configuration, layers/masks, and deterministic replay tests.
- [x] Add 2D physics broadphase, AABB/circle collision shape coverage, iterative solver configuration, and layers/masks.
- [x] Extend richer 3D shapes and stacked-body solver stability.
- [x] Decide whether to integrate a third-party physics backend for advanced 3D only after license/performance evaluation.
- [x] Add deterministic animation blend tree samples, layered animation samples, and float curve binding samples.
- [x] Add richer blend graphs, authored animation event tracks, and retargeting policy.
- [x] Add editor timeline controls.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` and GUI validation when editor surfaces change.

**Done when:** Runtime gameplay samples can use real audio output, robust collision, and authored animation behavior through public `mirakana::` APIs.

### Wave 7: Hot Reload and Live Iteration

**Files:**
- Modify: `engine/assets/include/mirakana/assets/asset_hot_reload.hpp`
- Modify: `engine/assets/src/asset_hot_reload.cpp`
- Modify: `engine/platform/include/mirakana/platform/file_watcher.hpp`
- Modify: `engine/platform/src/file_watcher.cpp`
- Modify: `engine/platform/include/mirakana/platform/windows_file_watcher.hpp`
- Modify: `engine/platform/src/windows_file_watcher.cpp`
- Create: `engine/platform/include/mirakana/platform/linux_file_watcher.hpp`
- Create: `engine/platform/src/linux_file_watcher.cpp`
- Create: `engine/platform/include/mirakana/platform/macos_file_watcher.hpp`
- Create: `engine/platform/src/macos_file_watcher.cpp`
- Modify: `engine/platform/include/mirakana/platform/filesystem.hpp`
- Modify: `engine/platform/src/filesystem.cpp`
- Modify: `engine/platform/CMakeLists.txt`
- Modify: `engine/tools/include/mirakana/tools/asset_file_scanner.hpp`
- Modify: `engine/tools/src/asset_file_scanner.cpp`
- Modify: `editor/core/include/mirakana/editor/workspace.hpp`
- Modify: `editor/core/src/workspace.cpp`
- Modify: `editor/src/main.cpp`
- Modify: `tests/unit/tools_tests.cpp`
- Modify: `tests/unit/core_tests.cpp`
- Modify: `tests/unit/platform_process_tests.cpp`
- Create: `tests/unit/platform_native_file_watcher_tests.cpp`
- Modify: `tests/unit/editor_core_tests.cpp`

**Steps:**
- [x] Add file watcher backend selection and a deterministic polling fallback behind platform contracts.
- [x] Add a Windows native `ReadDirectoryChangesW` file watcher backend behind the native selection path.
- [x] Add macOS and Linux native OS file watcher backends behind the native selection path.
- [x] Add debounced asset recook scheduling, dependency invalidation, and failed-reload rollback.
- [x] Add editor/core diagnostic state for changed, recook-requested, failed, and applied assets.
- [x] Add recook import plan extraction and editor recook execution through the asset import tool path.
- [x] Add runtime recook execution through the asset import tool path plus safe runtime asset replacement staging and safe-point commits.
- [x] Add shader hot reload through artifact provenance and backend pipeline recreation boundaries.
- [x] Add GUI diagnostics for changed, recooked, failed, and applied assets.
- [x] Add first-party texture/audio source byte payload and mesh vertex/index byte payload preservation into cooked artifacts.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

**Done when:** Editing source assets or shaders updates cooked artifacts and editor/runtime preview state without corrupting live scenes.

### Wave 8: Editor and Runtime UI Architecture

**Files:**
- Create: `engine/ui/include/mirakana/ui/ui.hpp`
- Create: `engine/ui/src/ui.cpp`
- Create: `engine/ui/CMakeLists.txt`
- Create: `editor/core/include/mirakana/editor/ui_model.hpp`
- Create: `editor/core/src/ui_model.cpp`
- Modify: `CMakeLists.txt`
- Modify: `docs/architecture.md`
- Modify: `docs/editor.md`
- Create: `docs/ui.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/dependencies.md`
- Modify: `docs/ai-game-development.md`
- Modify: `engine/agent/manifest.json`
- Modify: `AGENTS.md`
- Modify: `.claude/rules/cpp-engine.md`
- Modify: `.agents/skills/editor-change/SKILL.md`
- Modify: `.claude/skills/gameengine-editor/SKILL.md`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`
- Modify: `tests/unit/core_tests.cpp`
- Modify: `tests/unit/editor_core_tests.cpp`

**Steps:**
- [x] Record the UI ownership policy in `docs/ui.md`: first-party `MK_ui`/`MK_editor_ui` contracts, adapter-only low-level dependencies, Dear ImGui as developer shell, and middleware as optional adapters.
- [x] Add a `MK_ui` module skeleton with no SDL3, Dear ImGui, Qt, NoesisGUI, Slint, RmlUi, OS, or GPU dependencies.
- [x] Add retained-mode UI document primitives: stable element ids, parent/child hierarchy, visibility, enabled state, bounds, semantic role, and deterministic traversal.
- [x] Add style and layout contracts for rows, columns, stacks, anchors, margins, padding, min/max size, DPI scale, and theme tokens without binding to a renderer.
- [x] Add row/column gap support to the headless layout solver without coupling layout to renderer adapters.
- [x] Add text contracts for UTF-8 labels, font family references, text direction policy, truncation/wrapping mode, and localization keys without shipping a font renderer yet.
- [x] Add explicit adapter contracts for text shaping, bidirectional text, line breaking, font rasterization, glyph atlases, IME composition, OS accessibility bridges, image decoding, and renderer submission so future implementations can use official SDKs or audited dependencies without leaking them through `MK_ui`.
- [x] Add first-party renderer submission payload generation from solved UI layout, including submitted element bounds, `RendererBox` records, `RendererTextRun` records, and accessibility node payloads with localization key, enabled, and focusable metadata while keeping concrete renderer APIs behind adapters.
- [x] Add `MK_ui_renderer` as the first concrete first-party UI renderer adapter, converting styled `RendererBox` payloads into `MK_renderer` `SpriteCommand` submissions through theme tokens while keeping text/font/image/accessibility integration behind adapters.
- [x] Add input focus and navigation contracts for mouse, touch, keyboard, and gamepad, including active/focused/hovered state and modal layer routing.
- [x] Add animation state contracts for UI transitions without coupling to the renderer frame graph.
- [x] Add data binding primitives for editor/runtime UI models, including one-way display binding and command invocation.
- [x] Add `MK_editor_ui` model helpers in `editor/core` for inspector-like property rows, asset list rows, command palette entries, and diagnostics panels.
- [x] Keep the current Dear ImGui `MK_editor` shell as an adapter that can render existing immediate-mode panels while new complex panels are first modeled through `MK_editor_ui`.
- [x] Add tests for hierarchy determinism, focus routing, navigation, style resolution, layout constraints, command invocation, and editor UI model serialization.
- [x] Add a middleware and low-level UI dependency evaluation note covering Qt, NoesisGUI, Slint, RmlUi, font/text libraries, accessibility bridge strategy, and UI asset decoding; do not add dependencies until `license-audit`, `docs/dependencies.md`, `vcpkg.json`, and `THIRD_PARTY_NOTICES.md` are updated.
- [x] Update AI manifest, shared instructions, docs, and skills so generated games avoid Dear ImGui, SDL3, editor modules, and editor-private APIs for runtime HUD/menu code.
- [x] After concrete `MK_ui` public headers exist, update AI manifest and skills so generated games use those runtime HUD/menu contracts directly.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

**Done when:** Game runtime UI has a first-party retained-mode model that can be tested headlessly, AI agents can target it without private/editor APIs, low-level text/font/IME/accessibility/image/rendering integrations sit behind adapters, and the existing ImGui editor remains a developer shell rather than the only UI architecture.

### Wave 9: Mobile Packaging

**Files:**
- Create: `platform/android/`
- Create: `platform/ios/`
- Modify: `CMakeLists.txt`
- Modify: `CMakePresets.json`
- Modify: `engine/platform/include/mirakana/platform/lifecycle.hpp`
- Modify: `engine/platform/include/mirakana/platform/input.hpp`
- Modify: `engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_backend.hpp`
- Modify: `engine/rhi/metal/include/mirakana/rhi/metal/metal_backend.hpp`
- Modify: `tools/validate.ps1`
- Modify: `docs/workflows.md`
- Modify: `docs/architecture.md`
- Modify: `engine/agent/manifest.json`

**Steps:**
- [x] Add Android Gradle/NDK/GameActivity project template with CMake/Prefab integration.
- [x] Map mobile lifecycle, touch, safe area, storage, orientation, and permissions into first-party contracts without native handles.
- [x] Add iOS/macOS Apple packaging template with Metal framework linkage, lifecycle mapping, orientation, and bundle resources.
- [x] Add mobile package validation scripts that report missing SDK/toolchain blockers explicitly.
- [x] Add mobile sample game packaging scripts that validate an existing `games/<name>/game.agent.json` before Android/iOS package build attempts.
- [x] Add host-validated Android Debug APK build, Android native Vulkan surface creation bridge, private AAudio mobile output adapter, and resource packaging once Android SDK/NDK/JDK/Gradle are available.
- [x] Add Android Release signing validation with a real non-repository upload key and device/emulator smoke coverage.
- [ ] Add host-validated iOS simulator/device build, native Metal visible backend integration, signing, save data, and resource packaging once macOS/Xcode are available (Metal-backed view setup, sandbox save/cache/shared storage roots, bundle resources, manifest packaging, bundle identifier, simulator/device SDK selection, simulator signing suppression, Xcode signing inputs, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/smoke-ios-package.ps1` Simulator install/launch script, and `.github/workflows/ios-validate.yml` are implemented; actual macOS/Xcode CI results, device signing validation, and full backend-neutral Metal presentation remain Apple-host gated).

**Done when:** A generated C++ game can be packaged for Android and Apple targets through documented scripts without changing engine public APIs for each platform.

### Wave 10: SDK, Samples, and Release Packaging

**Files:**
- Modify: `cmake/GameEngineConfig.cmake.in`
- Modify: `CMakeLists.txt`
- Modify: `tools/package.ps1`
- Modify: `README.md`
- Modify: `docs/workflows.md`
- Modify: `docs/testing.md`
- Modify: `games/CMakeLists.txt`
- Create or modify: `examples/`
- Create or modify: `docs/release.md`

**Steps:**
- [x] Define installable SDK layout for headers, libraries, CMake config, tools, editor, samples, schemas, and AI manifests.
- [x] Add CPack artifact metadata and platform-specific package naming.
- [x] Add sample games that cover 2D arcade, 3D prototype, UI-heavy game, physics, audio, animation, and asset import workflows.
- [x] Add release validation that builds installed examples against the installed package instead of the source tree.
- [x] Publish CI artifacts for packages and logs.

**Done when:** A clean consumer project can build a game against the installed GameEngine SDK and AI agents can read the installed manifest/schema contract.

### Wave 11: AI-Driven Game Creation Excellence

**Files:**
- Modify: `engine/agent/manifest.json`
- Modify: `schemas/engine-agent.schema.json`
- Modify: `schemas/game-agent.schema.json`
- Modify: `tools/agent-context.ps1`
- Modify: `tools/new-game.ps1`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/specs/game-template.md`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.agents/skills/gameengine-agent-integration/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/rules/cpp-engine.md`
- Modify: `.codex/agents/*.toml`
- Modify: `.claude/agents/*.md`

**Steps:**
- [x] Extend engine/game manifests with runtime backend readiness, importer capabilities, packaging targets, and validation recipes.
- [x] Add generated-game validation scenarios for common genres and platform targets.
- [x] Add prompt packs and game templates that use only public engine APIs.
- [x] Keep Codex and Claude Code guidance behaviorally equivalent through validation checks.
- [x] Add agent-context output sections for public headers, module ownership, sample games, assets, and platform targets.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

**Done when:** Codex/GPT/Claude can inspect the engine, scaffold a game, pick supported systems, generate C++ gameplay code, run validation, and avoid non-existent or private APIs.

## Non-Negotiables

- Do not fake real rendering, audio output, physics backend capability, importer support, or mobile packaging in samples or agent guidance.
- Do not expose native `HWND`, `ID3D12*`, `Vk*`, `MTL*`, Objective-C, Android, or platform SDK handles through public engine APIs without an accepted interop design.
- Do not add `/std:c++latest` or lower the standard to C++20 to work around toolchain issues.
- Do not add third-party code, assets, shaders, fonts, or generated material without license and dependency records.
- Do not add compatibility shims for pre-release APIs; prefer clean replacement and update generated-game guidance.
- Do not make game projects parse external asset formats directly; import/cook through engine tools.

## Verification Matrix

- Default correctness: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`
- AI contract: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`
- JSON schema contract: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`
- Dependency policy: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1`
- C++23 policy: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-cpp-standard-policy.ps1`
- Windows release package: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/evaluate-cpp23.ps1 -Release`
- Optional desktop editor: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`
- Full local Windows confidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/evaluate-cpp23.ps1 -Release -Gui`
- Renderer backend tasks: add backend-specific tests plus `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` before claiming visible rendering support.
- Dependency/importer tasks: run `license-audit`, update records, then run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` and feature-specific importer validation.
