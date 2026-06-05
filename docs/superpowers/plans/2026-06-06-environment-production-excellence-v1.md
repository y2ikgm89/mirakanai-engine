# 2026-06-06 Environment Production Excellence v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` or `superpowers:executing-plans` after the operator explicitly selects this plan for implementation. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `environment-production-excellence-v1`

**Status:** Active. Selected milestone. Phase 0 selects this plan in `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`; runtime implementation has not started.

**Goal:** Promote the completed environment foundation into a production-grade, backend-evidence-backed environment settings system for sky, atmosphere, fog, volumetrics, clouds, rain, snow, storms, IBL, runtime cubemap capture, local volumes, material weathering, audio cues, editor authoring, quality budgets, and exact `environment_ready` aggregation.

**Architecture:** Keep `MK_environment` as the backend-neutral value, IO, profile, volume, weather, and quality contract. Keep scene adaptation in `MK_scene_renderer`, rendering policy in `MK_renderer`, and GPU execution in backend-private `MK_rhi_d3d12`, `MK_rhi_vulkan`, and Apple-host-gated `MK_rhi_metal` code. Promote claims only through exact package-visible counters, backend-local validation recipes, docs/manifest/static checks, and no native handle exposure.

**Tech Stack:** C++23, PowerShell 7 repository tools, `MK_environment`, `MK_scene`, `MK_scene_renderer`, `MK_renderer`, `MK_rhi`, `MK_rhi_d3d12`, strict host/toolchain-gated `MK_rhi_vulkan`, Apple-host-gated `MK_rhi_metal`, HLSL, DXC DXIL, DXC SPIR-V CodeGen, `spirv-val`, Vulkan 1.3 synchronization2 and dynamic rendering, Direct3D 12 descriptor/resource/barrier/debug-layer evidence, Apple Metal texture/pipeline/synchronization evidence, first-party `MK_editor_core` and `MK_editor`, no Dear ImGui, no SDL3.

---

## Current Audit Verdict

Audit sources used on 2026-06-06:

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1 -ContextProfile Minimal`
- `docs/current-capabilities.md`
- `docs/roadmap.md`
- `docs/superpowers/plans/README.md`
- `docs/superpowers/plans/2026-06-05-environment-rendering-readiness-v1.md`
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Targeted `rg` scans over `engine/environment`, `engine/renderer`, `engine/rhi`, `engine/runtime_host`, `editor/core`, `games/sample_desktop_runtime_game`, `tests/unit`, `shaders/environment`, and `tools`

Pre-Phase-0 audit proven state:

- Before this Phase 0 selection, `currentActivePlan` pointed to `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`.
- Before this Phase 0 selection, `recommendedNextPlan.id = next-production-gap-selection`.
- `unsupportedProductionGaps = []` was true before Phase 0 and remains true after Phase 0 selection.
- Environment System v1 and Environment Rendering Readiness v1 are completed.
- `MK_environment` owns profile validation, text IO, package rows, scene/runtime binding, time-of-day, weather, sky, fog, cloud, precipitation, volumetric, and quality value foundations.
- Selected D3D12 environment package or renderer evidence exists for physical sky, height fog, volumetric fog, cloud layer, rain precipitation, snow precipitation, volumetric clouds, and package-visible IBL metadata.
- Strict host/toolchain-gated Vulkan physical-sky runtime and height-fog package evidence exists, but Vulkan precipitation, volumetric cloud, volumetric fog broad readiness, and IBL proof remain unclaimed.
- Metal environment rows exist as Apple-host evidence requirements only; Windows validation does not promote Metal readiness.
- First-party editor Inspector rows expose selected environment ready/non-ready state without Dear ImGui, SDL3, UI middleware, backend execution, package-script execution, or public backend handles.
- Selected package smokes emit `environment_quality_budget_*` counters and keep `environment_quality_budget_broad_environment_ready_claimed=0`.

Unclaimed by design:

- Broad aggregate `environment_ready`.
- Broad environment backend parity.
- Broad CPU/GPU/memory optimization.
- Renderer cubemap upload and runtime cubemap capture.
- Vulkan/Metal precipitation execution.
- Vulkan/Metal volumetric fog and volumetric cloud execution.
- Vulkan/Metal IBL proof.
- Metal actual environment readiness without Apple-host execution.
- Material wetness and snow accumulation mutation.
- Audio playback execution for weather; existing rows are handoff or zero-playback evidence.
- OpenEXR/KTX/Basis/source-image dependencies for environment assets.

## Official Source Baseline

Re-open these sources before executing the phase that uses them. Do not copy sample code. Use official behavior and API constraints as the design input, then implement first-party code.

Product-scope references:

- Unreal Engine environmental light, fog, clouds, sky, and atmosphere: <https://dev.epicgames.com/documentation/en-us/unreal-engine/environmental-light-with-fog-clouds-sky-and-atmosphere-in-unreal-engine>
- Unreal Engine Volumetric Cloud component: <https://dev.epicgames.com/documentation/unreal-engine/volumetric-cloud-component-in-unreal-engine?lang=en-US>
- Unreal Engine Sky Atmosphere: <https://dev.epicgames.com/documentation/en-us/unreal-engine/sky-atmosphere?application_version=4.27>
- Unity HDRP Volume Profile and Volume Overrides: <https://docs.unity3d.com/ja/Packages/com.unity.render-pipelines.high-definition%4010.5/manual/Volume-Profile.html> and <https://docs.unity3d.com/ja/Packages/com.unity.render-pipelines.high-definition%4010.5/manual/Volume-Components.html>
- Godot stable Environment and post-processing: <https://docs.godotengine.org/en/stable/learning/features/3d/environment_and_post_processing.html>
- Godot volumetric fog and fog volumes: <https://docs.godotengine.org/en/latest/tutorials/3d/volumetric_fog.html>

Implementation-source references:

- Microsoft Direct3D 12 resource barriers: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12>
- Microsoft Direct3D 12 resource binding and descriptors: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/resource-binding-flow-of-control> and <https://learn.microsoft.com/en-us/windows/win32/direct3d12/descriptors-overview>
- Microsoft Direct3D 12 upload/readback: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/uploading-resources> and <https://learn.microsoft.com/en-us/windows/win32/direct3d12/readback-data-using-heaps>
- Microsoft Direct3D 12 debug layer and GPU-based validation: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/direct3d-12-sdklayers-interfaces> and <https://learn.microsoft.com/da-dk/windows/win32/direct3d12/using-d3d12-debug-layer-gpu-based-validation>
- Microsoft `D3D12_TEXCUBE_SRV`: <https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_texcube_srv>
- Khronos Vulkan synchronization and image layout transitions: <https://docs.vulkan.org/spec/latest/chapters/synchronization.html>
- Khronos Vulkan synchronization2 guide: <https://docs.vulkan.org/guide/latest/extensions/VK_KHR_synchronization2.html>
- Khronos Vulkan dynamic rendering: <https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/02_Graphics_pipeline_basics/03_Render_passes.html>
- Khronos Vulkan cube-map sampling: <https://docs.vulkan.org/spec/latest/chapters/textures.html>
- Khronos Vulkan validation layers: <https://docs.vulkan.org/guide/latest/layers.html>
- Apple Metal resource synchronization: <https://developer.apple.com/documentation/metal/resource-synchronization>
- Apple `MTLTextureType` cube texture rules: <https://developer.apple.com/documentation/metal/mtltexturetype>
- Apple `MTLTextureUsage` render target and shader read usage: <https://developer.apple.com/documentation/metal/mtltextureusage/rendertarget>
- Apple Metal Feature Set Tables: <https://developer.apple.com/metal/capabilities/>
- CMake generated file rules: <https://cmake.org/cmake/help/latest/guide/tutorial/Custom%20Commands%20and%20Generated%20Files.html> and <https://cmake.org/cmake/help/latest/command/add_custom_command.html>

Context7 checks performed on 2026-06-06:

- `/khronosgroup/vulkan-docs` confirmed synchronization2 image barrier patterns for compute-write to fragment-read, color attachment to shader-read, dynamic-rendering layout constraints, and validation-layer-driven evidence.
- `/websites/learn_microsoft_en-us` resolved Microsoft Learn but returned broad results; use the Microsoft Learn D3D12 pages listed above as the authoritative D3D12 source.
- `/websites/developer_apple_cn` resolved Apple Developer docs but returned broad results; use the Apple Developer Metal pages listed above as the authoritative Metal source.

Source implications for this plan:

- Unreal, Unity, and Godot are product-scope references only. They justify a volume/profile/editor-authoring model, but this repository implements first-party contracts.
- D3D12 evidence must track descriptor heap/table use, resource states, transitions, upload/readback heaps, fences, and debug-layer/GBV diagnostics in backend-private code.
- Vulkan evidence must use explicit image layouts, synchronization2 barriers, descriptor safe points, SPIR-V validation, validation layers, and strict host/toolchain gates.
- Metal evidence must run on Apple hosts with actual command queue, render/compute pipeline, texture, cube/HDR resource, and synchronization proof. Windows-side rows are host-gate declarations only.
- CMake shader/code generation must declare outputs, byproducts, dependencies, and `VERBATIM`; shared generated outputs need a single producer target to avoid races.

## Resolved Planning Decisions

- This is one milestone plan with multiple PR-sized phases, not one PR.
- Phase 0 selects the plan. Until this Phase 0 PR lands on `main`, mainline may still point at the previous selection gate.
- `GameEngine.EnvironmentProfile.v2` is the clean-break profile contract for this milestone. Execution must replace the selected sample profile/package assets instead of adding a v1 compatibility parser.
- D3D12 remains the primary Windows proof lane. Vulkan and Metal claims require their own backend-local rows and validation recipes.
- Local/global environment volumes are first-party profile rows, not editor-only concepts.
- IBL metadata is not renderer upload. Runtime cubemap capture and renderer cubemap upload are separate promotion rows.
- Rain and snow execution remain separate precipitation lanes. Snow surface accumulation is material-weathering work, not precipitation particle execution.
- Weather audio playback must execute through audio/runtime-host ownership. Environment code may emit trigger rows but must not directly own audio devices.
- Quality settings are explicit budgets and selected feature toggles, not broad optimization.
- Broad `environment_ready` is the final aggregate after exact rows pass. It is not a synonym for "some environment features are ready."
- Dear ImGui, SDL3, Qt, Slint, RmlUi, UI middleware, public native handles, and compatibility shims are forbidden for this milestone.
- OpenEXR, KTX, Basis, source-image decoder, or HDR importer dependencies are excluded unless a phase begins with `license-audit`, `vcpkg.json`, dependency docs, legal notices, package evidence, and validation.

## Deferred Broad Scope Gates

These items are included in this plan as explicit future gates so they are not mistaken for forgotten work. They are not required to complete `environment-production-excellence-v1`, and Phase 9 must not promote any of them unless a new focused plan selects the item and proves the exact claim.

| Item | Treatment in this plan | Required future selection evidence |
| --- | --- | --- |
| OpenEXR / KTX / Basis asset pipeline | Dependency-gated future asset-pipeline milestone, not a core environment renderer requirement. KTX/Basis is texture-container, compression, transcoding, and GPU-format capability work; OpenEXR is source/HDR image pipeline work. | `license-audit`; `vcpkg.json` manifest feature or documented non-vcpkg reason; `docs/dependencies.md`; `docs/legal-and-licensing.md`; `THIRD_PARTY_NOTICES.md`; importer/cooker tests; package smoke; platform format-selection evidence; no default validation dependency unless selected. |
| AAA-grade preset asset library | Product/content-pack milestone after asset pipeline, art direction, license provenance, storage/package budgets, and sample consumption are selected. It is not an engine-core readiness claim. | Source ownership and license records; asset naming/schema rules; package size and memory budgets; sample scenes; editor browsing/import evidence; package/install smoke; third-party notices for every distributable asset. |
| Complete weather fluid simulation | Advanced simulation/research milestone, not required for production environment settings. This plan covers authored weather state, volumetric/fog/cloud/precipitation execution, material response, audio cues, and quality budgets. | Numerical model spec; deterministic or explicitly nondeterministic contract; stability limits; CPU/GPU workload budget; fallback renderer path; validation images/data; package counters; profiler traces before any readiness claim. |
| Broad renderer quality | Separate renderer quality milestone. Environment-specific renderer proofs in this plan do not imply general renderer quality. | Renderer quality matrix; backend-local D3D12/Vulkan/Metal evidence; visual regression assets; shader/pipeline validation; package recipes; hosted checks for every claimed backend/host. |
| Broad CPU/GPU/memory optimization | Separate performance optimization milestone. This plan only adds scoped environment quality budgets and counters. | Exact subsystem, workload, host, metric, before/after traces, profiler tool, regression budget, package-visible counters, fallback path, and static checks preventing broad optimized claims from narrow evidence. |
| All-platform unconditional parity | Forbidden as an unconditional claim. Use backend and host exact gates only, and keep unsupported hosts unclaimed. | Per-platform and per-backend validation recipes for D3D12, Vulkan, Metal, Android, iOS, Linux, and macOS as selected; host/toolchain evidence; package smokes; manifest rows; no inference from another backend or host. |

## Target Claim Matrix

| Claim | Current state | Promotion target |
| --- | --- | --- |
| `environment_profile_v2_status` | Missing | Clean-break v2 profile rows with global profile, local volumes, priority, blend weight, weather timeline, quality preset, and deterministic package IO. |
| Local environment volumes | Missing | Box, sphere, and global rows with priority, fade distance, blend mode, deterministic sorted evaluation, diagnostics, and editor projection. |
| Runtime cubemap capture | Unclaimed | D3D12 primary offscreen six-face capture path with framegraph passes, readback checksum, zero native handle leakage, and package-visible counters. |
| Renderer cubemap upload | Unclaimed | D3D12 texture-cube upload/SRV/sampling proof with reflection, irradiance, radiance mip metadata, debug-layer clean run, and readback. |
| IBL Vulkan proof | Unclaimed | Strict Vulkan cube image/view/sampler/upload/readback proof with synchronization2 layout transitions and `spirv-val` artifacts. |
| IBL Metal proof | Host-gated | Apple-host texture cube/HDR/pipeline proof through `renderer-metal-apple-host-evidence`. |
| Material wetness | Unclaimed | First-party material-weathering rows, renderer material parameter binding, package counters, zero source-material mutation unless explicitly authored. |
| Snow accumulation | Unclaimed | First-party snow mask/depth/coverage rows, material parameter binding, package counters, and separate precipitation particle evidence. |
| Weather audio playback | Handoff only | Audio trigger rows consumed by runtime/audio lane with WASAPI package counters and no direct environment audio-device ownership. |
| Vulkan precipitation | Unclaimed | Strict particle texture/buffer upload, descriptor, depth-occlusion, draw/readback proof with validation layers. |
| Vulkan volumetric cloud/fog | Unclaimed | Strict compute/render paths with synchronization2 barriers, package recipes, positive dispatch/raymarch/readback counters. |
| Metal environment execution | Host-gated rows only | Apple-host feature-local render/compute pipeline, texture, cube/HDR, and synchronization evidence for selected features. |
| Editor environment authoring | Read-only readiness rows | First-party authoring panels for profile v2, volume stack, weather timeline, quality presets, and readiness reports without backend execution from editor core. |
| Quality budgets | Selected package counters | Profile-driven quality presets with fail-closed budgets for samples, raymarch steps, particles, transient GPU bytes, framegraph passes, barriers, and diagnostics. |
| Broad `environment_ready` | Unclaimed | Aggregate recipe only after all selected exact rows pass and static checks enforce the aggregate definition. |

## File Responsibility Map

Expected files for implementation phases:

| Area | Files |
| --- | --- |
| Environment values and IO | `engine/environment/include/mirakana/environment/environment_profile.hpp`, `engine/environment/include/mirakana/environment/environment_io.hpp`, `engine/environment/include/mirakana/environment/weather.hpp`, `engine/environment/include/mirakana/environment/time_of_day.hpp`, `engine/environment/src/environment_profile.cpp`, `engine/environment/src/environment_io.cpp`, `engine/environment/src/weather.cpp`, `engine/environment/src/time_of_day.cpp`, `tests/unit/environment_tests.cpp`, `tests/unit/environment_weather_tests.cpp`, `tests/unit/environment_time_of_day_tests.cpp` |
| Renderer policies | `engine/renderer/include/mirakana/renderer/environment_policy.hpp`, `engine/renderer/include/mirakana/renderer/environment_lighting_policy.hpp`, `engine/renderer/include/mirakana/renderer/physical_sky_policy.hpp`, `engine/renderer/include/mirakana/renderer/environment_fog_policy.hpp`, `engine/renderer/include/mirakana/renderer/cloud_layer_policy.hpp`, `engine/renderer/include/mirakana/renderer/precipitation_policy.hpp`, `engine/renderer/include/mirakana/renderer/volumetric_fog_policy.hpp`, `engine/renderer/include/mirakana/renderer/volumetric_cloud_policy.hpp`, matching `engine/renderer/src/*.cpp`, matching `tests/unit/renderer_*_tests.cpp` |
| RHI and backend execution | `engine/rhi/include/mirakana/rhi/rhi.hpp`, `engine/rhi/include/mirakana/rhi/upload_staging.hpp`, `engine/rhi/d3d12/src/d3d12_backend.cpp`, `engine/rhi/vulkan/src/vulkan_backend.cpp`, `engine/rhi/metal/src/metal_backend.cpp`, `engine/rhi/metal/src/metal_native.mm`, `tests/unit/d3d12_rhi_tests.cpp`, `tests/unit/backend_scaffold_tests.cpp`, `tests/unit/rhi_upload_staging_tests.cpp` |
| Runtime host package evidence | `engine/runtime_host/win32/src/win32_desktop_presentation.cpp`, `engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_desktop_presentation.hpp`, `games/sample_desktop_runtime_game/main.cpp`, `tools/package-desktop-runtime.ps1`, `tools/validate-installed-desktop-runtime.ps1`, `tools/validation-recipe-core.ps1`, `tools/run-validation-recipe-plans.ps1` |
| Shaders and generated artifacts | `shaders/environment/physical_sky.hlsl`, `shaders/environment/height_fog.hlsl`, `shaders/environment/cloud_layer.hlsl`, `shaders/environment/precipitation.hlsl`, `shaders/environment/volumetric_fog.hlsl`, `shaders/environment/volumetric_clouds.hlsl`, `tools/compile-vulkan-physical-sky-test-spirv.ps1`, `tools/compile-vulkan-height-fog-test-spirv.ps1`, new compile wrappers only when a strict Vulkan phase needs them |
| Sample package assets | `games/sample_desktop_runtime_game/source/environment/default_outdoor.geenv`, `games/sample_desktop_runtime_game/runtime/assets/desktop_runtime/default_outdoor.geenv`, `games/sample_desktop_runtime_game/source/textures/environment_ibl.texture_source`, `games/sample_desktop_runtime_game/runtime/assets/desktop_runtime/environment_ibl.texture.geasset`, `games/sample_desktop_runtime_game/game.agent.json`, `games/sample_desktop_runtime_game/README.md` |
| Editor authoring | `editor/core/include/mirakana/editor/environment_authoring.hpp`, `editor/core/src/environment_authoring.cpp`, `tests/unit/editor_environment_tests.cpp`, visible `MK_editor` files only after editor-core rows are green |
| Agent/docs/static checks | `engine/agent/manifest.fragments/009-validationRecipes.json`, `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`, `engine/agent/manifest.fragments/014-gameCodeGuidance.json`, composed `engine/agent/manifest.json`, `docs/current-capabilities.md`, `docs/roadmap.md`, `docs/ai-game-development.md`, `docs/testing.md`, `docs/superpowers/plans/README.md`, `docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md`, `tools/check-ai-integration-030-runtime-rendering.ps1`, `tools/check-json-contracts-030-tooling-contracts.ps1`, `tools/check-json-contracts-040-agent-surfaces.ps1` |

## Execution Strategy For Speed And Quality

- Use one isolated worktree per PR-sized phase.
- Use focused RED/GREEN loops first, then docs/manifest/static-check sync after behavior is green.
- Keep D3D12 primary lanes ahead of Vulkan/Metal only where shared policy/resource contracts are not yet proven.
- Dispatch independent read-only audits in parallel, but do not let parallel writers touch the same manifest/docs/static-check files at the same time.
- Run `tools/validate.ps1` once at each C++/runtime/build/packaging/public-contract phase close.
- Run `tools/check-publication-preflight.ps1` before staging, push, PR creation, ready conversion, auto-merge, or post-merge cleanup.
- Keep hosted checks authoritative for PR merge readiness. Do not push directly to `main`.

## Phase 0: Select The Milestone Without Implementing Claims

**Goal:** Make this plan the active selected environment milestone without changing runtime behavior.

**Files:**

- Modify `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Generate `engine/agent/manifest.json`
- Modify `docs/superpowers/plans/2026-06-06-environment-production-excellence-v1.md`
- Modify `docs/superpowers/plans/README.md`
- Modify `docs/current-capabilities.md` and `docs/roadmap.md` only for selected-plan context
- Modify static checks only if new selected-plan literals must be enforced

- [x] Confirm the worktree started from a clean baseline before Phase 0 edits.
- [x] Set `currentActivePlan` to `docs/superpowers/plans/2026-06-06-environment-production-excellence-v1.md`.
- [x] Set `recommendedNextPlan.id` to `environment-production-excellence-v1`.
- [x] Preserve `unsupportedProductionGaps = []`.
- [x] State explicitly that broad `environment_ready`, backend parity, broad optimization, material weathering, runtime cubemap capture, renderer cubemap upload, Vulkan/Metal precipitation, Vulkan/Metal volumetrics, and Metal readiness remain unclaimed.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1 -ContextProfile Minimal
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Expected: all checks pass and no C++ runtime behavior changes.

Phase 0 validation evidence on 2026-06-06:

- `tools/compose-agent-manifest.ps1 -Write`: pass.
- `tools/check-json-contracts.ps1`: pass.
- `tools/check-ai-integration.ps1`: pass.
- `tools/check-agents.ps1`: pass.
- `tools/check-text-format.ps1`: pass.
- `tools/agent-context.ps1 -ContextProfile Minimal`: pass.
- `tools/validate.ps1`: pass, including 99/99 CTest tests.

## Phase 1: Clean-Break Environment Profile v2 And Local Volumes

**Goal:** Replace selected sample environment profile usage with `GameEngine.EnvironmentProfile.v2`, including global settings, local volumes, deterministic blending, weather timeline, and quality presets.

**Files:** Environment values/IO, sample package assets, environment tests, package/runtime-scene/tool tests, docs/manifest/static checks. First-party editor authoring remains Phase 7.

- [x] Add RED tests for `GameEngine.EnvironmentProfile.v2` parse/serialize round trip.
- [x] Add RED tests for volume sorting: higher priority first, stable id tie-breaker, invalid shape diagnostics, invalid fade diagnostics, and deterministic blend hash.
- [x] Add RED tests for supported volume shapes: `global`, `box`, and `sphere`.
- [x] Add RED tests for weather timeline blending: time-of-day keyframes, weather preset keyframes, storm intensity, precipitation kind, cloud coverage, fog density, and quality preset selection.
- [x] Implement v2 profile value rows in `MK_environment`.
- [x] Replace selected sample `default_outdoor.geenv` source/runtime files with v2 text.
- [x] Remove active selected-package reliance on v1 profile parsing for this sample lane. Do not add a v1 compatibility parser.
- [x] Add package-visible counters:

```text
environment_profile_v2_status=ready
environment_profile_v2_ready=1
environment_profile_v2_volume_rows=<positive>
environment_profile_v2_weather_keyframes=<positive>
environment_profile_v2_quality_preset=<low|medium|high|ultra|custom>
environment_profile_v2_diagnostics=0
environment_profile_v2_legacy_v1_accepted=0
```

- [x] Run focused tests:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_environment_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "environment"
```

Expected: environment, asset, runtime-scene, and tool tests pass; selected package smoke proves v2 evidence and rejects legacy v1 acceptance. Editor authoring remains a Phase 7 claim.

Phase 1 focused validation evidence on 2026-06-06:

- `tools/cmake.ps1 --build --preset dev --target MK_environment_tests MK_assets_tests MK_runtime_scene_tests MK_tools_tests`: pass for all test targets; `sample_desktop_runtime_game` is not a `dev` preset target.
- `tools/ctest.ps1 --preset dev --output-on-failure -R "^(MK_environment_tests|MK_assets_tests|MK_runtime_scene_tests|MK_tools_tests)$"`: pass, 4/4 tests.
- `tools/cmake.ps1 --preset desktop-runtime`: pass.
- `tools/cmake.ps1 --build --preset desktop-runtime --target sample_desktop_runtime_game`: pass.
- `tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game -SmokeArgs @('--smoke','--max-frames','2','--require-config','runtime/sample_desktop_runtime_game.config','--require-scene-package','runtime/sample_desktop_runtime_game.geindex','--require-environment-profile')`: pass; smoke reports `environment_profile_v2_status=ready`, `environment_profile_v2_ready=1`, `environment_profile_v2_volume_rows=2`, `environment_profile_v2_weather_keyframes=3`, `environment_profile_v2_quality_preset=high`, `environment_profile_v2_diagnostics=0`, and `environment_profile_v2_legacy_v1_accepted=0`.

## Phase 2: D3D12 IBL Renderer Upload And Runtime Cubemap Capture

**Goal:** Promote selected D3D12 IBL from package metadata to renderer/RHI upload and runtime cubemap capture evidence.

**Files:** `environment_lighting_policy`, `upload_staging`, D3D12 backend, Win32 desktop presentation, sample package, D3D12 tests, shaders if capture needs a fixture shader.

- [ ] Add RED D3D12 tests for texture-cube upload with 6 faces, positive edge size, positive mip count, supported HDR format, SRV type `TextureCube`, shader sampling, and CPU readback checksum.
- [ ] Add RED policy tests separating reflection cubemap upload, irradiance rows, radiance mip rows, runtime capture, and HDR clamp rows.
- [ ] Add RED package smoke assertions for:

```text
environment_lighting_renderer_upload_status=ready
environment_lighting_texture_cube_uploads>0
environment_lighting_texture_cube_faces=6
environment_lighting_radiance_mips>=5
environment_lighting_irradiance_rows=9
environment_lighting_backend_invocations>0
environment_lighting_runtime_captures>0
environment_lighting_runtime_capture_faces=6
environment_lighting_runtime_capture_readback_nonzero=1
environment_lighting_native_handle_access=0
```

- [ ] Implement backend-private D3D12 upload, SRV, framegraph capture pass, and readback proof.
- [ ] Add validation recipe `desktop-runtime-sample-game-environment-ibl-renderer-execution`.
- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset desktop-runtime
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset desktop-runtime --target sample_desktop_runtime_game
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset desktop-runtime --output-on-failure -R "d3d12|sample_desktop_runtime_game"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game -SmokeArgs @("--require-environment-lighting-renderer-execution")
```

Expected: D3D12 IBL renderer upload and runtime cubemap capture are ready for the selected package lane only.

## Phase 3: Material Wetness And Snow Accumulation

**Goal:** Add first-party material-weathering rows driven by environment state without mutating source material documents implicitly.

**Files:** `MK_environment`, `MK_renderer` material/environment policy files, sample runtime package, material tests, renderer tests, package validation.

- [ ] Add RED tests for material-weathering policy rows: `dry`, `wet`, `snow_covered`, `icy`, and `mixed`.
- [ ] Add RED tests that source `.material` content is not rewritten by runtime weather.
- [ ] Add RED renderer policy tests for weathering constant rows and material parameter binding.
- [ ] Add package counters:

```text
environment_material_weathering_status=ready
environment_material_weathering_ready=1
environment_material_weathering_wet_rows>0
environment_material_weathering_snow_rows>0
environment_material_weathering_source_material_mutations=0
environment_material_weathering_backend_invocations>0
environment_material_weathering_native_handle_access=0
```

- [ ] Implement value-only `MK_environment` weathering inputs, renderer policy rows, and selected D3D12 material parameter binding evidence.
- [ ] Keep precipitation particle execution and snow surface accumulation as separate counters.
- [ ] Run focused renderer/material tests and the selected package smoke.

Expected: wetness and snow accumulation are renderer material state evidence, not source asset mutation.

## Phase 4: Weather Audio Playback Execution

**Goal:** Promote weather audio from handoff rows to selected runtime audio playback evidence through existing audio/runtime-host ownership.

**Files:** `MK_audio`, `MK_runtime_host`, sample package, `games/sample_desktop_runtime_game/main.cpp`, package validation, docs/manifest.

- [ ] Add RED tests for environment-to-audio trigger planning: rain loop, snow ambience, thunder one-shot, wind loop.
- [ ] Add RED tests that `MK_environment` does not own audio devices and only emits value rows.
- [ ] Add package counters:

```text
environment_audio_playback_status=ready
environment_audio_playback_ready=1
environment_audio_trigger_rows=4
environment_audio_runtime_cues_started>0
environment_audio_runtime_cues_stopped>=0
environment_audio_device_owned_by_environment=0
environment_audio_native_handle_access=0
```

- [ ] Execute playback only through the existing audio/runtime-host surface and selected package lane.
- [ ] Keep missing WASAPI/device host evidence as `host_gated`, not `ready`.

Expected: weather audio playback is proven only where the selected host/runtime audio lane can execute it.

## Phase 5: Strict Vulkan Environment Feature Execution

**Goal:** Add strict Vulkan execution for selected environment features without inferring parity from D3D12.

**Files:** Vulkan backend, renderer policies, shader compile wrappers, `tests/unit/backend_scaffold_tests.cpp`, package recipes, sample manifest/docs.

- [ ] Add SPIR-V artifact compile wrappers only for selected features that need new fixtures.
- [ ] Add RED Vulkan tests for precipitation particle/depth/sampler descriptors, volumetric fog compute to shader-read transition, volumetric cloud texture sampling/raymarch, and IBL cube sampling.
- [ ] Use synchronization2 image barriers for compute-write to fragment-read and transfer-write to shader-read transitions.
- [ ] Require `spirv-val`, DXC SPIR-V CodeGen, Vulkan runtime, and validation-layer readiness.
- [ ] Add strict package recipes:

```text
desktop-runtime-sample-game-vulkan-environment-precipitation-renderer-execution
desktop-runtime-sample-game-vulkan-volumetric-fog-renderer-execution
desktop-runtime-sample-game-vulkan-volumetric-cloud-renderer-execution
desktop-runtime-sample-game-vulkan-environment-ibl-renderer-execution
```

- [ ] Add backend-local counters with `vulkan_` prefixes and `native_handle_access=0`.
- [ ] Preserve Metal and broad backend parity non-claims.

Expected: Vulkan rows become ready only when strict Vulkan host/toolchain/package proof passes.

## Phase 6: Apple Metal Host Evidence Execution

**Goal:** Convert selected Metal environment host-gated rows into Apple-host evidence where actual Metal execution exists.

**Files:** Metal backend, Metal native implementation, Apple host validation scripts, manifest recipes, docs/static checks.

- [ ] Extend `renderer-metal-apple-host-evidence` to cover selected environment features one at a time.
- [ ] Require Apple host evidence for `MTLCommandQueue`, non-empty MSL/metallib, render pipeline, compute pipeline where needed, cube/HDR texture, render target or shader-read usage, and synchronization proof.
- [ ] Add feature-local counters:

```text
metal_environment_physical_sky_status
metal_environment_height_fog_status
metal_environment_cloud_layer_status
metal_environment_precipitation_status
metal_environment_volumetric_fog_status
metal_environment_volumetric_cloud_status
metal_environment_lighting_ibl_status
metal_environment_native_handle_access=0
```

- [ ] Keep each missing feature as `host_evidence_required` with a concrete blocker string.
- [ ] Run on Apple host:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-apple-host-evidence.ps1 -RequireReady
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-renderer-metal-apple.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe renderer-metal-apple-host-evidence
```

Expected: Metal readiness is promoted only by Apple-host evidence.

## Phase 7: First-Party Editor Environment Authoring

**Goal:** Provide usable first-party editor authoring for EnvironmentProfile v2, local volumes, weather timeline, quality presets, IBL/capture status, and exact non-claims.

**Files:** `editor/core/include/mirakana/editor/environment_authoring.hpp`, `editor/core/src/environment_authoring.cpp`, `tests/unit/editor_environment_tests.cpp`, visible `MK_editor` Inspector/Project Settings surfaces after core rows pass.

- [ ] Add RED editor-core tests for profile v2 rows, volume stack rows, weather timeline rows, quality preset rows, backend readiness rows, and aggregate non-claim rows.
- [ ] Add explicit reviewed commands for add/remove/reorder volume rows, edit weather keyframes, select quality preset, and request cubemap capture.
- [ ] Keep package-script execution, validation recipe execution, backend execution, and native handles outside editor core.
- [ ] Surface visible controls through first-party `MK_editor` only after editor-core model tests pass.
- [ ] Keep `editor_shell_imgui=0` and `editor_shell_sdl3=0` evidence intact.

Expected: editor authoring is usable and AI-operable without reintroducing middleware or backend handles.

## Phase 8: Environment Quality Budgets And Profiling Evidence

**Goal:** Turn quality presets into fail-closed budgets and narrow profiling evidence without broad optimization claims.

**Files:** Environment quality policy, renderer policies, package validation, performance docs/manifest rows.

- [ ] Add quality preset rows: `low`, `medium`, `high`, `ultra`, `custom`.
- [ ] Add per-preset budgets for physical-sky samples, fog raymarch steps, cloud raymarch steps, precipitation particles, transient GPU bytes, framegraph passes, barrier steps, texture uploads, draw calls, compute dispatches, and diagnostics.
- [ ] Add RED tests for budget failure diagnostics and package smoke rejection.
- [ ] Add package counters:

```text
environment_quality_preset=<low|medium|high|ultra|custom>
environment_quality_budget_status=ready
environment_quality_budget_ready=1
environment_quality_budget_violations=0
environment_quality_budget_broad_optimization_claimed=0
```

- [ ] Attach D3D12/Vulkan/Metal timing or profiler evidence only through backend-specific rows when the host supports it.
- [ ] Do not claim broad optimization or cross-backend performance parity.

Expected: quality presets are enforceable budgets, not marketing labels.

## Phase 9: Exact `environment_ready` Aggregate

**Goal:** Add the broad aggregate only after all selected exact rows have evidence.

**Files:** package validation, sample manifest, docs, manifest fragments, static checks.

- [ ] Add aggregate validator requiring every selected exact row from this plan.
- [ ] Add package flag `--require-environment-ready-aggregate`.
- [ ] Add package counters:

```text
environment_ready_status=ready
environment_ready=1
environment_ready_profile_v2=1
environment_ready_d3d12_primary=1
environment_ready_vulkan_strict=<0|1>
environment_ready_metal_host=<0|1>
environment_ready_backend_parity=<0|1>
environment_ready_broad_optimization_claimed=0
environment_ready_native_handle_access=0
```

- [ ] If Vulkan or Metal is not selected for the aggregate host, the aggregate must say `backend_parity=0` and must not claim cross-backend parity.
- [ ] Add static checks so future docs cannot claim `environment_ready` without the aggregate definition and package evidence.
- [ ] Close the milestone by returning `currentActivePlan` to the master selection gate or selecting the next active plan.

Expected: `environment_ready` has one exact, enforceable meaning.

## Required Validation At Phase Close

Every C++/runtime/build/packaging/public-contract phase must run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset desktop-runtime
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset desktop-runtime --target sample_desktop_runtime_game
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Preset desktop-runtime -Files <changed-cpp-files>
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Before staging, push, PR creation, ready conversion, auto-merge, or merged worktree cleanup:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1
```

Docs-only planning edits may use narrower checks:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
```

## Non-Goals

- No Dear ImGui, SDL3, Qt, Slint, RmlUi, or UI middleware.
- No public D3D12, Vulkan, Metal, Win32, RHI, audio device, descriptor, command queue, fence, or native handle exposure.
- No backward-compatibility parser or shim for the selected EnvironmentProfile v2 path.
- No default validation dependency on OpenEXR, KTX, Basis, CUDA, HIP, SYCL, vendor profilers, Apple-only tools, or Android tools.
- No OpenEXR/KTX/Basis importer, source-image decoder, texture-compression toolchain, or HDR asset pipeline without the dependency/legal/package gate in this plan.
- No AAA-grade preset asset library, large content-pack production, art-direction system, or third-party asset distribution claim.
- No complete physical weather fluid simulation, fluid solver, CFD-style weather model, or research simulation readiness claim.
- No broad renderer quality claim from environment-specific renderer evidence.
- No broad optimization, backend parity, Metal readiness, Vulkan readiness, or `environment_ready` claim without exact evidence.
- No unconditional all-platform parity. Every platform/backend must have its own selected validation recipe and host/toolchain evidence.
- No material source mutation from runtime weather unless a future authored-material mutation plan explicitly selects it.
- No audio device ownership in environment code.

## Completion Definition

The milestone is complete only when:

- Phase 0 has selected the plan or the operator explicitly keeps it as proposed.
- Every selected implementation phase has passing focused tests, package smokes, docs, manifest fragments, static checks, and full `tools/validate.ps1` evidence.
- Hosted PR checks pass for each merged PR.
- `currentActivePlan`, `recommendedNextPlan`, registry, roadmap, current capabilities, master-plan backlog, manifest fragments, composed manifest, and static checks all describe the same ready/non-ready truth.
- Broad `environment_ready` is either still unclaimed with explicit counters, or claimed only through Phase 9 aggregate evidence.
- The deferred broad scope gates remain recorded as future work unless separately selected: OpenEXR/KTX/Basis asset pipeline, AAA preset asset library, complete weather fluid simulation, broad renderer quality, broad CPU/GPU/memory optimization, and all-platform unconditional parity.
- No untracked temporary files, generated scratch output, stale branch/worktree assumptions, Dear ImGui/SDL3 dependencies, native handle leaks, or dependency/legal drift remain.

## Plan Self-Review

- No implementation is performed by this plan creation.
- No active manifest pointer is changed by this proposed plan file.
- No broad ready claim is added by this proposed plan file.
- Host-gated work has explicit host requirements instead of vague blockers.
- Each promotion target has concrete status-line counters.
- Official source links and Context7 checks are recorded for refresh during implementation.
