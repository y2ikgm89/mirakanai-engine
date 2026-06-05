# Environment Rendering Readiness v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `environment-rendering-readiness-v1`

**Status:** Active. This is a clean-break post-Environment-System follow-up, not an Engine 1.0 blocker. Tasks 1-7 now have selected D3D12 or strict host-gated evidence as recorded below; Task 8 now has Windows-side Metal environment feature host-gated rows, while Apple-host execution and remaining backend-local Vulkan/Metal feature proofs remain future task selections.

**Goal:** Turn the completed Environment System v1 foundation into evidence-backed environment rendering readiness for snow, sky package proof, cloud and precipitation renderer execution, volumetric cloud/fog package proof, and backend-local D3D12/Vulkan/Metal claims without broad or inferred readiness.

**Architecture:** Keep `MK_environment` as backend-neutral value and IO ownership, keep `MK_scene_renderer` as scene-to-render-packet adaptation, and put rendering execution inside `MK_renderer` / `MK_rhi` / backend-private adapters. Each feature promotes only after tests, package-visible counters, docs, manifest rows, and validation recipes prove that exact claim; D3D12, Vulkan, and Metal evidence must stay backend-local.

**Tech Stack:** C++23, PowerShell 7 repository tools, `MK_environment`, `MK_scene`, `MK_scene_renderer`, `MK_renderer`, `MK_rhi`, `MK_rhi_d3d12`, `MK_rhi_vulkan`, host-gated `MK_rhi_metal`, HLSL, DXC DXIL, DXC SPIR-V CodeGen, `spirv-val`, Vulkan 1.3 synchronization2, Direct3D 12 root signatures/descriptors/resource barriers, Apple Metal host gates, first-party Win32 desktop runtime packaging, first-party `MK_editor_core`.

---

## Current Audit Verdict

The 2026-06-05 audit used `tools/agent-context.ps1 -ContextProfile Minimal`, the composed manifest, the plan registry, `docs/current-capabilities.md`, `docs/roadmap.md`, `docs/ai-game-development.md`, `docs/superpowers/plans/2026-05-26-environment-system-v1.md`, `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`, `008-packagingTargets.json`, `009-validationRecipes.json`, and targeted `rg` scans over environment, renderer, runtime-host, tests, tools, and package validation.

Proven today before this plan:

- `MK_environment` profile validation, deterministic `GameEngine.EnvironmentProfile.v1` text IO, cooked environment profile package rows, scene/runtime-scene environment references, scene-renderer environment render packets, renderer environment policy planning, physical-sky and height-fog shader contracts, cloud-layer policy rows, precipitation/weather value rows, volumetric-cloud/fog policy rows, time-of-day/weather-blend value rows, and first-party `MK_editor_core` environment authoring exist.
- Selected D3D12 physical-sky, height-fog, and volumetric-fog execution/readback evidence exists.
- Selected D3D12 height-fog package evidence exists through `desktop-runtime-sample-game-environment-fog-package` and `--require-environment-fog-evidence`.
- Selected D3D12 cloud-layer package evidence exists through `desktop-runtime-sample-game-cloud-layer-package` and `--require-cloud-layer-package-evidence`, but it is package-visible policy evidence only.
- Selected D3D12 rain precipitation package evidence exists through `desktop-runtime-sample-game-environment-precipitation-package` and `--require-environment-precipitation-package-evidence`, but it is rain package-visible policy evidence only.
- Selected D3D12 snow precipitation package evidence exists through `desktop-runtime-sample-game-environment-snow-package` and `--require-environment-snow-package-evidence`, but it is snow package-visible policy evidence only.
- Strict host/toolchain/env-gated Vulkan height-fog runtime readback proof exists.
- Dear ImGui and SDL3 are not active environment/editor/runtime dependencies and must remain absent.

Unclaimed and selected by this plan:

- Physical-sky Vulkan proof and package-visible physical-sky proof.
- Cloud-layer Vulkan proof and Metal cloud proof; selected D3D12 cloud-layer renderer/RHI execution is complete through Task 4.
- Vulkan/Metal precipitation renderer/RHI particle-buffer upload and draw execution.
- Volumetric-cloud execution and package readiness is complete for the selected D3D12 package lane through Task 6; Vulkan and Metal remain unclaimed.
- Environment lighting/IBL package proof is complete for the selected D3D12 package-visible first-party metadata lane through Task 7; renderer cubemap upload, runtime capture, Vulkan, and Metal remain unclaimed.
- Backend parity for environment features; each backend remains feature-local and evidence-local.
- Broad optimization, broad renderer quality, broad `environment_ready`, and inferred Vulkan/Metal readiness.

Completed by Task 3:

- Strict host/toolchain-gated Vulkan height-fog package readiness.
- Selected D3D12 volumetric-fog package readiness.
- Selected D3D12 snow precipitation package readiness, separate from the rain lane and still without renderer/RHI particle-buffer upload, material mutation, or audio playback.

Completed by Task 4:

- Selected D3D12 cloud-layer renderer/RHI execution with package cloud-map upload, a distinct flow-map package asset, backend-private descriptors/root signatures, positive renderer draw counters, and zero native-handle access. Vulkan and Metal cloud-layer execution remain unclaimed until backend-local proof runs.

Completed by Task 5:

- Selected D3D12 rain and snow precipitation renderer/RHI execution with target-specific precipitation DXIL artifacts, cooked particle texture upload, backend invocation, renderer draw counters, and depth-occlusion readback evidence. Vulkan and Metal precipitation execution remain unclaimed until backend-local proof runs.

Completed by Task 6:

- Selected D3D12 volumetric-cloud package evidence and renderer/RHI execution with target-specific DXIL artifacts, first-party weather/shape/erosion package evidence, positive texture upload/backend invocation/draw/raymarch/readback counters for execution, and zero native-handle/audio/precipitation counters. Vulkan and Metal volumetric-cloud execution remain unclaimed until backend-local proof runs.

Completed by Task 7:

- Selected D3D12 environment lighting / IBL package-visible evidence with first-party cooked texture package records, source registry rows, explicit HDR metadata, reflection cubemap metadata, irradiance rows, radiance mip rows, HDR clamp rows, and zero renderer upload/backend invocation/runtime-capture/native-handle counters. Runtime cubemap capture, renderer/RHI cubemap upload, Vulkan, Metal, backend parity, and broad `environment_ready` remain unclaimed.

Completed by Task 8 on Windows:

- Metal environment feature host-gated evidence rows now exist in `MK_rhi_metal` for physical sky, height fog, cloud layer, precipitation, volumetric fog, volumetric cloud, and environment lighting/IBL. These rows stay `host_evidence_required` until the reviewed `renderer-metal-apple-host-evidence` recipe runs on an Apple host with runtime, command queue, non-empty `metallib`, feature-local render/compute pipeline, render-pass, depth/particle, cube-map, and HDR texture evidence. Apple-host execution remains unclaimed on this Windows host.

## Official Source Baseline

Before executing any task, re-open the current official documents for that task. The 2026-06-05 planning audit used:

- Unreal Engine Volumetric Cloud Component: <https://dev.epicgames.com/documentation/unreal-engine/volumetric-cloud-component-in-unreal-engine?lang=en-US>
- Unreal Engine Sky Atmosphere Component: <https://dev.epicgames.com/documentation/unreal-engine/sky-atmosphere-component-in-unreal-engine?lang=en-US>
- Unreal Engine Volumetric Fog: <https://dev.epicgames.com/documentation/unreal-engine/volumetric-fog-in-unreal-engine?lang=en-US>
- Unreal Engine Niagara Overview: <https://dev.epicgames.com/documentation/en-us/unreal-engine/overview-of-niagara-effects-for-unreal-engine>
- Unity HDRP Volumetric Clouds: <https://docs.unity.cn/Packages/com.unity.render-pipelines.high-definition%4017.0/manual/Override-Volumetric-Clouds.html>
- Unity Visual Effect Graph Contexts: <https://docs.unity3d.com/Packages/com.unity.visualeffectgraph%4017.0/manual/Contexts.html>
- Microsoft Direct3D 12 Root Signatures overview: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/root-signatures-overview>
- Microsoft Direct3D 12 resource binding overview: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/resource-binding-flow-of-control>
- Microsoft Direct3D 12 `ResourceBarrier`: <https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist-resourcebarrier>
- Microsoft Direct3D 12 `D3D12_TEXCUBE_SRV` / texture subresource upload documentation for future cubemap upload proof.
- Apple Metal render pass configuration: <https://developer.apple.com/documentation/metal/render-pass-configuration>
- Apple Metal Feature Set Tables: <https://developer.apple.com/metal/capabilities/>
- Context7 `/khronosgroup/vulkan-docs` for Vulkan 1.3 synchronization2 image barriers, shader-read transitions, render/compute ordering, and validation-layer evidence.
- Context7 `/godotengine/godot-docs` for Environment fog, volumetric fog, `GPUParticles3D`, and `ParticleProcessMaterial` capability separation.
- Context7 `/godotengine/godot-docs` for sky radiance and irradiance separation, sky bake panorama/cubemap cache behavior, and ambient/reflection lighting terminology.
- Context7 `/microsoft/directxshadercompiler` for DXC DXIL/SPIR-V target profiles, `-spirv`, `-fspv-target-env`, and CI shader evidence.

Source implications for this repo:

- Volumetric clouds are a separate 3D volume/raymarch path, not a promoted form of the cheap 2D cloud layer.
- Rain and snow must be treated as particle lifecycle plus render-output execution, not only weather value rows.
- Fog and volumetric fog need explicit density/scattering, temporal, lighting, and depth/volume-resource evidence.
- D3D12 root signatures, descriptor tables/heaps, resource states, barriers, and fences stay backend-private and must be validated with focused readback/package tests before promotion.
- IBL package evidence is not cubemap upload evidence. Reflection cubemap metadata, irradiance coefficients, radiance mips, and HDR clamp policy can be validated as package-visible rows, but renderer upload requires a later backend-local texture-cube SRV/upload/readback proof.
- Vulkan feature claims require synchronization2 layout transitions, descriptor updates, shader-read barriers, SPIR-V validation, `VK_LAYER_KHRONOS_validation`, and host/toolchain gates.
- Metal claims require Apple-host pipeline/render/compute proof and feature-set availability; Windows validation can define host-gated rows but cannot promote Metal readiness.

## Clean-Break Constraints

- Do not add compatibility aliases, migration shims, deprecated names, or duplicate public APIs.
- Do not expose D3D12, Vulkan, Metal, RHI, Win32, Dear ImGui, SDL3, or other native handles through public environment/gameplay APIs.
- Do not add Dear ImGui, SDL3, Qt, Slint, RmlUi, or UI middleware.
- Do not add OpenEXR/KTX/Basis/source-image dependencies unless that task updates `vcpkg.json`, `docs/dependencies.md`, `docs/legal-and-licensing.md`, and `THIRD_PARTY_NOTICES.md` with package evidence.
- Do not promote a broad `environment_ready` aggregate unless every constituent feature row in this plan has ready evidence and static checks assert the aggregate meaning.
- Do not infer Vulkan or Metal from D3D12. Each backend row needs its own validation recipe and evidence.
- Do not mutate materials for wetness or snow accumulation until a selected material/surface plan proves it.
- Do not play audio from environment code. Audio remains handoff rows unless a selected audio execution plan proves playback.

## Claim Matrix

| Claim | Current state | Required promotion evidence |
| --- | --- | --- |
| `environment_profile_status=ready` | Ready for selected cooked profile package rows. | Keep existing profile counters; no new broad claim. |
| `environment_fog_status=ready` | Ready for selected D3D12 height-fog package; strict host/toolchain-gated Vulkan package lane is ready through `environment_fog_vulkan_package_status=ready`. | Keep Metal host-gated and do not infer backend parity. |
| physical sky package | Unclaimed. | D3D12 package smoke with shader artifact, constants, readback/package counters, and no broad sky readiness. |
| physical sky Vulkan | Unclaimed. | Strict Vulkan runtime readback with SPIR-V artifacts, synchronization2 barriers, validation layer, and package recipe only after runtime proof. |
| cloud layer renderer/RHI execution | Ready for selected D3D12 package renderer execution only; Vulkan and Metal remain unclaimed. | Keep positive D3D12 package counters for texture upload/backend invocation/renderer draw over package cloud-map and distinct flow-map assets with zero native handle leakage; add Vulkan strict proof and Metal host proof separately if selected. |
| precipitation renderer/RHI execution | Ready for selected D3D12 rain and snow package renderer execution only; Vulkan and Metal remain unclaimed. | Keep positive D3D12 package counters for particle-buffer upload, backend invocation, renderer draw, and depth-occlusion readback over rain and snow lanes; add Vulkan strict proof and Metal host proof separately if selected. |
| snow package readiness | Ready for selected D3D12 package-visible policy evidence only. | Keep `environment_precipitation_weather=snow`, `environment_precipitation_kind=snow`, zero wetness rows, one audio handoff row, and zero particle-buffer upload/backend invocation/material mutation/audio playback counters. |
| volumetric fog package | Ready for selected D3D12 package through `environment_volumetric_fog_status=ready` and positive `environment_volumetric_fog_compute_dispatches`. | Add Vulkan/Metal volumetric-fog lanes only after backend-local proof; do not infer volumetric-cloud readiness. |
| volumetric clouds | Ready for selected D3D12 package-visible evidence and selected D3D12 renderer/RHI execution only; Vulkan and Metal remain unclaimed. | Keep Task 6 package and execution recipe counters separate; add Vulkan/Metal lanes only after backend-local proof. |
| environment lighting/IBL | Ready for selected D3D12 package-visible first-party IBL metadata evidence only; renderer cubemap upload/runtime capture remain unclaimed. | Keep first-party cooked texture package record/source/HDR metadata rows, reflection cubemap face/edge/mip/format rows, irradiance/radiance rows, HDR clamp rows, and zero upload/backend/runtime-capture counters; add renderer upload proof in a separate backend-local task. |
| environment backend parity | Unclaimed. | Backend-local matrix rows for each feature; no single backend can promote another. |
| broad optimization | Unclaimed. | Separate profiling/budget plan with before/after traces and budgets. |
| broad `environment_ready` | Unclaimed. | Only after all rows above are ready and an aggregate static contract is accepted. |

## File Responsibility Map

These are the files expected to change during execution. A phase may use fewer files if the RED tests prove a narrower path is enough.

| Area | Files |
| --- | --- |
| Environment value contracts | `engine/environment/include/mirakana/environment/*.hpp`, `engine/environment/src/*.cpp`, `tests/unit/environment_*_tests.cpp` |
| Renderer policies | `engine/renderer/include/mirakana/renderer/*environment*`, `*cloud*`, `*precipitation*`, `*fog*`, `engine/renderer/src/*.cpp`, `tests/unit/renderer_*_tests.cpp` |
| RHI execution | `engine/rhi*/`, `engine/renderer/src/rhi*`, `tests/unit/d3d12_rhi_tests.cpp`, `tests/unit/backend_scaffold_tests.cpp`, Vulkan env-gated tests |
| Shaders | `shaders/environment/*.hlsl`, `tests/shaders/environment_*.hlsl`, shader compile scripts under `tools/` when new fixtures are required |
| Runtime package evidence | `engine/runtime_host_win32*/`, `tools/package-desktop-runtime.ps1`, `tools/validate-installed-desktop-runtime.ps1`, `tools/validation-recipe-core.ps1`, `tools/run-validation-recipe-plans.ps1` |
| Game/sample package rows | `games/sample_desktop_runtime_game/game.agent.json`, sample runtime package files under `games/sample_desktop_runtime_game/runtime/` |
| Editor authoring | `editor/core/include/mirakana/editor/environment_authoring.hpp`, `editor/core/src/environment_authoring.cpp`, first-party `MK_editor` visible shell files only after editor-core evidence is stable |
| Docs/agent surfaces | `docs/current-capabilities.md`, `docs/roadmap.md`, `docs/ai-game-development.md`, `docs/testing.md`, plan registry, master-plan backlog, `engine/agent/manifest.fragments/*.json`, static checks |

## Task 0: Selection, Audit, And Static Contract Sync

**Files:**
- Create: `docs/superpowers/plans/2026-06-05-environment-rendering-readiness-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/ai-game-development.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `tools/check-json-contracts-030-tooling-contracts.ps1`
- Modify: `tools/check-json-contracts-040-agent-surfaces.ps1`
- Modify: `tools/check-ai-integration-010-agent-baseline.ps1`
- Modify: `tools/check-ai-integration-020-engine-manifest.ps1`
- Modify: `tools/check-ai-integration-030-runtime-rendering.ps1`
- Generate: `engine/agent/manifest.json`

- [x] **Step 1: Confirm current truth before selection**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1 -ContextProfile Minimal
git status --short --branch
```

Expected:

- `currentActivePlan` points to `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`.
- `recommendedNextPlan.id = next-production-gap-selection`.
- `unsupportedProductionGaps = []`.
- The worktree is clean except for task-owned edits.

- [x] **Step 2: Select this plan in manifest fragments**

Update `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`:

- `currentActivePlan = docs/superpowers/plans/2026-06-05-environment-rendering-readiness-v1.md`
- `recommendedNextPlan.id = environment-rendering-readiness-v1`
- `recommendedNextPlan.title = Environment Rendering Readiness v1`
- `recommendedNextPlan.status = selected-production-slice`
- `recommendedNextPlan.path = docs/superpowers/plans/2026-06-05-environment-rendering-readiness-v1.md`
- `unsupportedProductionGaps = []`

Expected: Environment System v1 remains completed evidence; this plan is a post-1.0 clean-break selected slice.

- [x] **Step 3: Compose and guard static contracts**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
```

Expected: all commands pass after this planning-only selection.

- [x] **Step 4: Record planning evidence**

In this task section, record the exact commands that passed and state that no C++ runtime behavior was changed.

Task 0 evidence recorded on 2026-06-05:

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1 -ContextProfile Minimal` confirmed the pre-selection state: `currentActivePlan` was the production-completion master plan, `recommendedNextPlan.id` was `next-production-gap-selection`, `unsupportedProductionGaps = []`, and `MK_environment` was already `implemented-environment-profile-v1-foundation`.
- `git status --short --branch` was clean on `main...origin/main` before edits; after this task it contains only task-owned planning, docs, manifest, and static-contract edits.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` passed and regenerated `engine/agent/manifest.json`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed with `json-contract-check: ok`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed with `ai-integration-check: ok`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` passed with `agent-config-check: ok`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1` passed with `text-format-check: ok`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1` passed with `toolchain-check: ok`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed with `public-api-boundary-check: ok`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with `validate: ok`; CTest reported `100% tests passed, 0 tests failed out of 99`. Apple/Metal lanes remained explicit host-gated diagnostics on this Windows host.
- No C++ runtime, renderer, shader, RHI, package, editor behavior, or public engine API was implemented or changed in Task 0.

## Task 1: Physical Sky Package And Vulkan Proof

**Files:**
- Modify: `engine/renderer/include/mirakana/renderer/physical_sky_policy.hpp`
- Modify: `engine/renderer/src/physical_sky_policy.cpp`
- Modify: `tests/unit/renderer_physical_sky_policy_tests.cpp`
- Modify: `tests/unit/d3d12_rhi_tests.cpp`
- Modify: `tests/unit/backend_scaffold_tests.cpp`
- Modify: `shaders/environment/physical_sky.hlsl`
- Modify: `tests/shaders/environment_physical_sky.hlsl`
- Modify: `tools/validate-installed-desktop-runtime.ps1`
- Modify: `tools/validation-recipe-core.ps1`
- Modify: `tools/run-validation-recipe-plans.ps1`
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/current-capabilities.md`

- [x] **Step 1: Re-open official sources**

Re-open Unreal Sky Atmosphere, the Hillaire paper link retained by Environment System v1, Microsoft D3D12 root/resource/barrier docs, Context7 `/khronosgroup/vulkan-docs`, and Context7 `/microsoft/directxshadercompiler`.

Expected: task notes name the exact official URLs and the DXC/Vulkan validation requirements used.

Evidence: Task 1 reopened Unreal Sky Atmosphere official documentation (`https://dev.epicgames.com/documentation/en-us/unreal-engine/sky-atmosphere?application_version=4.27`), the Hillaire/Hosek-style production atmosphere reference retained by the plan (`https://diglib.eg.org/items/8a3e5350-18b3-46bd-9274-3add5af88c75`), Microsoft D3D12 resource binding / root signature / barrier documentation, Context7 `/khronosgroup/vulkan-docs` for synchronization2 image barriers, validation layer, fence wait, and non-coherent readback requirements, and Context7 `/microsoft/directxshadercompiler` for DXC `-spirv`, `-fspv-target-env`, and target profile requirements.

- [x] **Step 2: Add RED package tests**

Add tests that require physical-sky package counters:

- `environment_physical_sky_status=ready`
- `environment_physical_sky_selected_backend=d3d12`
- `environment_physical_sky_shader_contract_evidence_ready=1`
- `environment_physical_sky_execution_evidence_ready=1`
- `environment_physical_sky_package_evidence_ready=1`
- `environment_physical_sky_constants_binding` equals the existing stable binding
- `environment_physical_sky_native_handle_access=0`
- no `environment_ready`

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_physical_sky_policy_tests MK_d3d12_rhi_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "physical_sky|d3d12_rhi"
```

Expected: RED fails on missing package promotion fields or missing Vulkan proof rows, not on unrelated compile errors.

Evidence: RED tests were added in `tests/unit/renderer_physical_sky_policy_tests.cpp` and `tests/unit/runtime_host_win32_public_api_compile.cpp`. The first focused build failed on missing physical-sky package promotion fields (`request_ready_promotion`, package/execution evidence fields, `ready()` diagnostics, and runtime-host report/evaluator fields), then passed after the policy/runtime host API was implemented.

- [x] **Step 3: Promote D3D12 package proof only**

Add the smallest D3D12 package-visible physical-sky lane using the existing packed-constants fullscreen readback evidence. Keep Vulkan and Metal fields host-gated or unclaimed.

Expected: installed package smoke reports physical-sky fields and still reports no broad `environment_ready`.

Evidence: focused configure/build/CTest passed after enabling the desktop runtime lane:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev -DMK_ENABLE_DESKTOP_RUNTIME=ON
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_physical_sky_policy_tests MK_runtime_host_win32_public_api_compile
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "physical_sky|runtime_host_win32_public_api_compile"
```

The selected D3D12 package smoke passed through `tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game -RequireD3d12Shaders` with `--require-physical-sky-package-evidence`, proving `environment_physical_sky_status=ready`, `environment_physical_sky_ready=1`, D3D12 selected backend, shader-contract/package/execution evidence, constants binding `0`, constants byte size `256`, eight constant-layout rows, four LUT-intent rows, zero diagnostics, zero LUT texture allocation/backend invocation/native-handle counters, and no broad `environment_ready` row.

- [x] **Step 4: Add strict Vulkan runtime proof**

Compile target-specific SPIR-V from the physical-sky shader contract, validate with `spirv-val --target-env vulkan1.3`, and add an env-gated Vulkan readback test with synchronization2 barriers and `VK_LAYER_KHRONOS_validation`.

Expected: Vulkan proof is a separate row such as `environment_physical_sky_vulkan_runtime_status=ready` only when env vars and host gates are satisfied.

Evidence: `shaders/environment/physical_sky.hlsl` now carries explicit Vulkan binding metadata for the physical-sky constants buffer. `tools/compile-vulkan-physical-sky-test-spirv.ps1` generated `MK_VULKAN_TEST_PHYSICAL_SKY_VERTEX_SPV` / `MK_VULKAN_TEST_PHYSICAL_SKY_FRAGMENT_SPV` artifacts with DXC `-spirv -fspv-target-env=vulkan1.3`, and `spirv-val --target-env vulkan1.3` passed. `MK_backend_scaffold_tests` now contains a host/toolchain/env-gated Vulkan readback proof that requires real SPIR-V artifacts, `VK_LAYER_KHRONOS_validation` instance-layer readiness, private synchronization2 command submission/barriers through the Vulkan RHI bridge, physical-sky constants binding `0`, and CPU readback assertions. This is a runtime proof only; Vulkan package readiness remains unclaimed.

Validation passed:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compile-vulkan-physical-sky-test-spirv.ps1
$env:MK_VULKAN_TEST_PHYSICAL_SKY_VERTEX_SPV = "<repo>\\out\\vulkan-physical-sky-test-artifacts\\environment_physical_sky.vs.spv"
$env:MK_VULKAN_TEST_PHYSICAL_SKY_FRAGMENT_SPV = "<repo>\\out\\vulkan-physical-sky-test-artifacts\\environment_physical_sky.fs.spv"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_backend_scaffold_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "backend_scaffold"
```

- [x] **Step 5: Validate**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game -RequireD3d12Shaders -SmokeArgs @('--smoke','--max-frames','2','--require-config','runtime/sample_desktop_runtime_game.config','--require-scene-package','runtime/sample_desktop_runtime_game.geindex','--require-d3d12-renderer','--require-physical-sky-package-evidence')
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
```

Expected: D3D12 physical-sky package passes; Vulkan package remains unclaimed unless an explicit strict Vulkan package recipe also passes.

Evidence: final Task 1 validation passed on 2026-06-05:

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1` exited 0 with D3D12 DXIL ready, Vulkan SPIR-V ready, DXC SPIR-V CodeGen ready, and Metal diagnostic-only host gates on Windows.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compile-vulkan-physical-sky-test-spirv.ps1` regenerated the physical-sky VS/FS SPIR-V artifacts and `spirv-val` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_physical_sky_policy_tests MK_runtime_host_win32_public_api_compile MK_backend_scaffold_tests` passed.
- With `MK_VULKAN_TEST_PHYSICAL_SKY_VERTEX_SPV` and `MK_VULKAN_TEST_PHYSICAL_SKY_FRAGMENT_SPV` pointing at the regenerated artifacts, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "physical_sky|runtime_host_win32_public_api_compile|backend_scaffold"` passed 3/3.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Recipe desktop-runtime-sample-game-physical-sky-package -Mode Execute -HostGateAcknowledgements d3d12-windows-primary -TimeoutSeconds 900` returned `status=passed`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-agents.ps1`, `tools/check-format.ps1`, and `tools/check-public-api-boundaries.ps1` passed after docs/manifest/static-check sync.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with `validate: ok`; CTest reported `100% tests passed, 0 tests failed out of 115`.

Task 1 promotes selected D3D12 package-visible physical-sky evidence and strict env-gated Vulkan physical-sky runtime readback evidence only. It does not claim Vulkan physical-sky package readiness, Metal readiness, backend parity, broad optimization, broad sky readiness, or broad `environment_ready`.

## Task 2: Height Fog Vulkan Package And Volumetric Fog Package

**Files:**
- Modify: `engine/renderer/include/mirakana/renderer/environment_fog_policy.hpp`
- Modify: `engine/renderer/include/mirakana/renderer/volumetric_fog_policy.hpp`
- Modify: `engine/renderer/src/environment_fog_policy.cpp`
- Modify: `engine/renderer/src/volumetric_fog_policy.cpp`
- Modify: `tests/unit/renderer_environment_fog_policy_tests.cpp`
- Modify: `tests/unit/renderer_volumetric_fog_policy_tests.cpp`
- Modify: `tests/unit/d3d12_rhi_tests.cpp`
- Modify: `tests/unit/backend_scaffold_tests.cpp`
- Modify: `tools/compile-vulkan-height-fog-test-spirv.ps1`
- Modify: `tools/validate-installed-desktop-runtime.ps1`
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`
- Modify: `docs/testing.md`

- [x] **Step 1: Add RED Vulkan package assertions**

Require a strict package recipe that names the existing height-fog Vulkan SPIR-V env vars and fails closed when `spirv-val`, DXC SPIR-V CodeGen, Vulkan runtime, or `VK_LAYER_KHRONOS_validation` is unavailable.

Expected: `environment_fog_vulkan_package_status=host_evidence_required` on unsupported hosts and `ready` only after strict smoke.

Evidence: Task 2 added public runtime-host compile assertions and installed validator expectations for `environment_fog_vulkan_package_status=ready`, `environment_fog_vulkan_package_ready=1`, `environment_fog_vulkan_selected_backend=vulkan`, shader-contract/package/depth/postprocess evidence, stable constants binding `4`, constants byte size `256`, zero diagnostics, and zero native-handle counters. The strict package smoke passed with:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -Command "& .\tools\package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game -RequireVulkanShaders -SmokeArgs @('--smoke','--max-frames','2','--require-config','runtime/sample_desktop_runtime_game.config','--require-scene-package','runtime/sample_desktop_runtime_game.geindex','--require-vulkan-scene-shaders','--require-vulkan-renderer','--require-scene-gpu-bindings','--require-postprocess','--require-postprocess-depth-input','--require-vulkan-postprocess-evidence','--require-environment-fog-vulkan-package-evidence')"
```

- [x] **Step 2: Add volumetric fog package counters**

Promote existing D3D12 volumetric-fog compute readback into package-visible counters:

- `environment_volumetric_fog_status=ready`
- `environment_volumetric_fog_selected_backend=d3d12`
- `environment_volumetric_fog_froxel_output_ready=1`
- `environment_volumetric_fog_scene_depth_ready=1`
- `environment_volumetric_fog_compute_dispatches>0`
- `environment_volumetric_fog_native_handle_access=0`

Expected: this does not claim Vulkan or Metal volumetric fog.

Evidence: Task 2 promoted selected D3D12 volumetric-fog package counters only. The installed smoke passed with:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -Command "& .\tools\package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game -RequireD3d12Shaders -SmokeArgs @('--smoke','--max-frames','2','--require-config','runtime/sample_desktop_runtime_game.config','--require-scene-package','runtime/sample_desktop_runtime_game.geindex','--require-d3d12-scene-shaders','--require-d3d12-renderer','--require-scene-gpu-bindings','--require-postprocess','--require-postprocess-depth-input','--require-d3d12-postprocess-evidence','--require-environment-volumetric-fog-package-evidence')"
```

The ready claim is limited to `environment_volumetric_fog_status=ready`, `environment_volumetric_fog_selected_backend=d3d12`, froxel output and scene-depth evidence, positive `environment_volumetric_fog_compute_dispatches`, stable constants binding `5`, froxel output binding `13`, zero diagnostics, and zero native-handle counters.

- [x] **Step 3: Validate**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_environment_fog_policy_tests MK_renderer_volumetric_fog_policy_tests MK_d3d12_rhi_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "environment_fog|volumetric_fog|d3d12_rhi"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
```

Expected: D3D12 volumetric fog and host-gated Vulkan height-fog package rows are exact and narrow.

Evidence: focused validation passed for:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools\cmake.ps1 --build --preset dev --target MK_renderer_environment_fog_policy_tests MK_renderer_volumetric_fog_policy_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools\ctest.ps1 --preset dev --output-on-failure -R "environment_fog|volumetric_fog"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools\cmake.ps1 --preset desktop-runtime
pwsh -NoProfile -ExecutionPolicy Bypass -File tools\cmake.ps1 --build --preset desktop-runtime --target sample_desktop_runtime_game MK_runtime_host_win32_public_api_compile
pwsh -NoProfile -ExecutionPolicy Bypass -File tools\ctest.ps1 --preset desktop-runtime --output-on-failure -R "MK_runtime_host_win32_public_api_compile"
```

Final docs/manifest/static validation for Task 2 is recorded in this candidate's closeout checklist after manifest compose and `tools/validate.ps1`.

## Task 3: Snow Package Readiness

**Files:**
- Modify: `engine/environment/include/mirakana/environment/weather.hpp`
- Modify: `engine/environment/src/weather.cpp`
- Modify: `engine/renderer/include/mirakana/renderer/precipitation_policy.hpp`
- Modify: `engine/renderer/src/precipitation_policy.cpp`
- Modify: `tests/unit/environment_weather_tests.cpp`
- Modify: `tests/unit/renderer_precipitation_policy_tests.cpp`
- Modify: `tools/validate-installed-desktop-runtime.ps1`
- Modify: `tools/validation-recipe-core.ps1`
- Modify: `tools/run-validation-recipe-plans.ps1`
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`
- Modify: `engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_desktop_presentation.hpp`
- Modify: `engine/runtime_host/win32/src/win32_desktop_presentation.cpp`
- Modify: `games/sample_desktop_runtime_game/main.cpp`
- Modify: `games/sample_desktop_runtime_game/game.agent.json`
- Modify: `games/sample_desktop_runtime_game/README.md`
- Modify: `docs/ai-game-development.md`

- [x] **Step 1: Re-open official particle/VFX sources**

Re-open Unity VFX Graph Contexts, Unreal Niagara Overview, Godot `GPUParticles3D` / `ParticleProcessMaterial`, and existing Context7 Godot particle notes.

Expected: the snow package keeps particle lifecycle, render policy, material mutation, and audio playback separated.

- [x] **Step 2: Add RED snow package tests**

Require a selected package smoke for snow:

- `environment_precipitation_status=ready`
- `environment_precipitation_selected_backend=d3d12`
- `environment_precipitation_weather=snow`
- `environment_precipitation_kind=snow`
- `environment_precipitation_particle_rows=1`
- `environment_precipitation_wetness_rows=0`
- `environment_precipitation_audio_handoff_rows>=1`
- `environment_precipitation_particle_buffer_uploads=0` until Task 5
- `environment_precipitation_backend_invocations=0` until Task 5
- `environment_precipitation_material_mutations=0`
- `environment_precipitation_audio_playback=0`

Expected: RED fails because the current package lane proves only storm/rain.

- [x] **Step 3: Add snow package lane**

Add a package-visible snow lane with snow texture/reference rows and separate smoke args such as `--require-environment-snow-package-evidence`. Reuse existing weather value rows and avoid material accumulation.

Expected: snow becomes package-ready while renderer/RHI precipitation execution remains unclaimed.

- [x] **Step 4: Validate**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_environment_weather_tests MK_renderer_precipitation_policy_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "environment_weather|precipitation_policy"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game -RequireD3d12Shaders -SmokeArgs @('--smoke','--max-frames','2','--require-config','runtime/sample_desktop_runtime_game.config','--require-scene-package','runtime/sample_desktop_runtime_game.geindex','--require-d3d12-scene-shaders','--require-d3d12-renderer','--require-scene-gpu-bindings','--require-postprocess','--require-postprocess-depth-input','--require-d3d12-postprocess-evidence','--require-environment-snow-package-evidence')
```

Expected: snow package evidence passes and rain evidence still passes.

Evidence: Context7 `/godotengine/godot-docs` re-checked `GPUParticles3D` / `ParticleProcessMaterial` emitter-vs-process separation. `MK_environment_weather_tests`, `MK_renderer_precipitation_policy_tests`, and `MK_runtime_host_win32_public_api_compile` passed. `tools/package-desktop-runtime.ps1` passed both selected D3D12 snow `--require-environment-snow-package-evidence` and rain `--require-environment-precipitation-package-evidence` package smokes. `tools/check-json-contracts.ps1`, `tools/check-validation-recipe-runner.ps1`, `tools/check-ai-integration.ps1`, `tools/check-format.ps1`, `tools/check-agents.ps1`, `tools/check-public-api-boundaries.ps1`, `tools/check-native-desktop-contracts.ps1`, and full `tools/validate.ps1` passed on 2026-06-05.

## Task 4: Cloud Layer Renderer/RHI Execution

**Files:**
- Modify: `engine/environment/include/mirakana/environment/cloud_layer.hpp`
- Modify: `engine/environment/src/cloud_layer.cpp`
- Modify: `engine/renderer/include/mirakana/renderer/cloud_layer_policy.hpp`
- Modify: `engine/renderer/src/cloud_layer_policy.cpp`
- Modify: `engine/renderer/src/rhi_frame_renderer*`
- Modify: `engine/rhi_d3d12/*`
- Modify: `tests/unit/environment_cloud_tests.cpp`
- Modify: `tests/unit/renderer_cloud_layer_policy_tests.cpp`
- Modify: `tests/unit/d3d12_rhi_tests.cpp`
- Modify: `tests/unit/backend_scaffold_tests.cpp`
- Modify: `shaders/environment/cloud_layer.hlsl`
- Modify: `tests/shaders/environment_cloud_layer.hlsl`
- Modify: `tools/validate-installed-desktop-runtime.ps1`

- [x] **Step 1: Add RED execution tests**

Require cloud-map SRV, distinct flow-map SRV, sampler, constants, fullscreen or sky-pass draw, and readback evidence. Assert package counters move from policy-only:

- `cloud_layer_texture_uploads>0`
- `cloud_layer_backend_invocations>0`
- `cloud_layer_renderer_draws>0`
- `cloud_layer_native_handle_access=0`
- `cloud_layer_volumetric_clouds=0`

Expected: RED fails because the package-only counters are `texture_uploads=0`, `backend_invocations=0`, and `renderer_draws=0`.

- [x] **Step 2: Implement D3D12 execution behind RHI**

Route cloud layer drawing through backend-private descriptors/root signatures/resource barriers. Keep public APIs on `CloudLayerPolicyPlan` and renderer packets.

Expected: D3D12 package smoke proves positive texture-upload, backend-invocation, and renderer-draw counters from the cloud-layer shader path.

- [ ] **Step 3: Add strict Vulkan proof**

Add SPIR-V fixtures and a Vulkan env-gated readback using synchronization2 transitions for color/depth reads as required by the pass.

Expected: `cloud_layer_vulkan_runtime_status=ready` only when strict host/toolchain gates pass. The current candidate does not claim this row.

- [x] **Step 4: Validate**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_cloud_layer_policy_tests MK_d3d12_rhi_tests MK_backend_scaffold_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "cloud_layer|d3d12_rhi|backend_scaffold"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game -RequireD3d12Shaders -SmokeArgs @('--smoke','--max-frames','2','--require-config','runtime/sample_desktop_runtime_game.config','--require-scene-package','runtime/sample_desktop_runtime_game.geindex','--require-d3d12-scene-shaders','--require-d3d12-renderer','--require-scene-gpu-bindings','--require-postprocess','--require-postprocess-depth-input','--require-d3d12-postprocess-evidence','--require-cloud-layer-renderer-execution')
```

Expected: cloud-layer renderer execution is D3D12-ready only; Vulkan/Metal rows stay feature-local.

Evidence recorded on 2026-06-05:

- Context7 `/websites/learn_microsoft_en-us_windows_win32_direct3d12` rechecked Microsoft Learn render-pass and graphics PSO format contracts. The selected D3D12 fix keeps the cloud-layer PSO depth format compatible with the active scene render pass when `--require-postprocess-depth-input` is enabled.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-shader-toolchain.ps1` passed as diagnostic-only with `d3d12_dxil=ready`, `vulkan_spirv=ready`, and Windows-host Metal toolchain blockers (`metal` / `metallib` missing).
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\cmake.ps1 --build --preset dev --target MK_renderer_cloud_layer_policy_tests MK_d3d12_rhi_tests MK_backend_scaffold_tests` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\ctest.ps1 --preset dev --output-on-failure -R "cloud_layer|d3d12_rhi|backend_scaffold"` passed with 3/3 tests.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game -RequireD3d12Shaders -SmokeArgs @('--smoke','--max-frames','2','--require-config','runtime/sample_desktop_runtime_game.config','--require-scene-package','runtime/sample_desktop_runtime_game.geindex','--require-d3d12-scene-shaders','--require-d3d12-renderer','--require-scene-gpu-bindings','--require-postprocess','--require-postprocess-depth-input','--require-d3d12-postprocess-evidence','--require-cloud-layer-renderer-execution')` passed.
- Direct installed smoke with the same renderer-execution flags exited 0 and reported `cloud_layer_texture_uploads=1`, `cloud_layer_backend_invocations=1`, and `cloud_layer_renderer_draws=2`.

## Task 5: Precipitation Renderer/RHI Execution For Rain And Snow

**Files:**
- Modify: `engine/renderer/include/mirakana/renderer/precipitation_policy.hpp`
- Modify: `engine/renderer/src/precipitation_policy.cpp`
- Modify: `engine/renderer/src/rhi_frame_renderer*`
- Modify: `engine/rhi_d3d12/*`
- Modify: `engine/rhi_vulkan/*`
- Modify: `tests/unit/renderer_precipitation_policy_tests.cpp`
- Modify: `tests/unit/d3d12_rhi_tests.cpp`
- Modify: `tests/unit/backend_scaffold_tests.cpp`
- Modify: `shaders/environment/precipitation.hlsl`
- Modify: `tests/shaders/environment_precipitation.hlsl`
- Modify: `tools/validate-installed-desktop-runtime.ps1`

- [x] **Step 1: Add RED particle execution tests**

Require particle instance buffer upload, particle texture SRV, scene-depth SRV, sampler, constants, draw call count, depth occlusion readback, and package counters:

- `environment_precipitation_particle_buffer_uploads>0`
- `environment_precipitation_backend_invocations>0`
- `environment_precipitation_renderer_draws>0`
- `environment_precipitation_depth_occlusion_readback=1`
- `environment_precipitation_material_mutations=0`
- `environment_precipitation_audio_playback=0`
- `environment_precipitation_native_handle_access=0`

Expected: RED fails because current precipitation evidence is package-visible policy only.

- [x] **Step 2: Implement D3D12 rain execution**

Add camera-near sprite particle draw execution with deterministic seed/instance rows and scene-depth occlusion. Keep wetness and audio as rows only.

Expected: D3D12 rain execution passes without changing material or audio state.

- [x] **Step 3: Extend to snow execution**

Use snow package rows from Task 3, lower fall-speed behavior, snow texture rows, and no wetness mutation.

Expected: D3D12 snow execution passes separately from rain.

- [ ] **Step 4: Add strict Vulkan proof**

Compile SPIR-V and add an env-gated Vulkan particle draw/readback path with synchronization2 transitions for depth read and color output.

Expected: Vulkan ready rows are separate from D3D12.

- [x] **Step 5: Validate selected D3D12 rain/snow execution**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_precipitation_policy_tests MK_d3d12_rhi_tests MK_backend_scaffold_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "precipitation|d3d12_rhi|backend_scaffold"
```

Expected: rain and snow renderer execution are feature-local; sleet, hail, dust, and ash remain value/package rows unless selected separately.

Evidence recorded on 2026-06-05:

- Context7 `/websites/learn_microsoft_en-us_windows_win32_direct3d12` rechecked Microsoft Learn resource-barrier, render-target/depth clear, descriptor-heap/table, draw, and readback guidance. The selected D3D12 probe transitions depth textures from depth write to shader read before sampling and transitions color targets from render target to copy source before readback.
- `MK_renderer_precipitation_policy_tests` and `MK_d3d12_rhi_tests` focused validation passed during Task 5 development, proving policy promotion counters, target-specific precipitation shader compilation, particle instance input layout, sampled scene-depth binding, and visible precipitation readback.
- `execute_precipitation_runtime_probe` now uses separate unoccluded and occluded scene-depth textures plus separate descriptor sets and render-target readbacks. It promotes `environment_precipitation_depth_occlusion_readback=1` only when the unoccluded center alpha is visible and the occluded center alpha is suppressed, so the claim proves scene-depth occlusion rather than only depth sampling.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\cmake.ps1 --build --preset desktop-runtime --target MK_runtime_host_win32_public_api_compile sample_desktop_runtime_game` passed after the depth-occlusion probe fix.
- `pwsh -NoProfile -ExecutionPolicy Bypass -Command "& .\tools\package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game -RequireD3d12Shaders -SmokeArgs @('--smoke','--max-frames','2','--require-config','runtime/sample_desktop_runtime_game.config','--require-scene-package','runtime/sample_desktop_runtime_game.geindex','--require-d3d12-scene-shaders','--require-d3d12-renderer','--require-scene-gpu-bindings','--require-postprocess','--require-postprocess-depth-input','--require-d3d12-postprocess-evidence','--require-environment-precipitation-renderer-execution')"` passed with `installed-desktop-runtime-validation: ok (sample_desktop_runtime_game)` and `desktop-runtime-package: ok (sample_desktop_runtime_game)`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -Command "& .\tools\package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game -RequireD3d12Shaders -SmokeArgs @('--smoke','--max-frames','2','--require-config','runtime/sample_desktop_runtime_game.config','--require-scene-package','runtime/sample_desktop_runtime_game.geindex','--require-d3d12-scene-shaders','--require-d3d12-renderer','--require-scene-gpu-bindings','--require-postprocess','--require-postprocess-depth-input','--require-d3d12-postprocess-evidence','--require-environment-snow-renderer-execution')"` passed with `installed-desktop-runtime-validation: ok (sample_desktop_runtime_game)` and `desktop-runtime-package: ok (sample_desktop_runtime_game)`.
- `--require-environment-precipitation-renderer-execution` and `--require-environment-snow-renderer-execution` remain selected D3D12-only package smokes. Vulkan precipitation execution, Metal precipitation execution, sleet/hail/dust/ash, material wetness mutation, audio playback, native-handle access, backend parity, broad optimization, and broad `environment_ready` remain unclaimed.

## Task 6: Volumetric Cloud Execution And Package Readiness

**Files:**
- Modify: `engine/renderer/include/mirakana/renderer/volumetric_cloud_policy.hpp`
- Modify: `engine/renderer/src/volumetric_cloud_policy.cpp`
- Modify: `engine/rhi_d3d12/*`
- Modify: `engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_desktop_presentation.hpp`
- Modify: `engine/runtime_host/win32/src/win32_desktop_presentation.cpp`
- Modify: `games/CMakeLists.txt`
- Modify: `games/sample_desktop_runtime_game/main.cpp`
- Modify: `engine/rhi_vulkan/*`
- Modify: `tests/unit/renderer_volumetric_cloud_tests.cpp`
- Modify: `tests/unit/d3d12_rhi_tests.cpp`
- Modify: `shaders/environment/volumetric_clouds.hlsl`
- Modify: `tools/validate-installed-desktop-runtime.ps1`
- Modify: `tools/validation-recipe-core.ps1`
- Modify: `tools/run-validation-recipe-plans.ps1`
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`
- Modify: `games/sample_desktop_runtime_game/game.agent.json`

- [x] **Step 1: Add RED volumetric-cloud package and D3D12 execution tests**

Require package references for weather map, shape noise, erosion noise, sampler, constants, raymarch step budget, lighting rows, temporal settings, and shadow rows.

Result: renderer policy tests now require package evidence before ready promotion, reject execution claims without concrete upload/backend/raymarch/readback counters, and verify the exact 256-byte constant layout. `MK_d3d12_rhi_tests` adds public Texture3D upload/readback proof plus a D3D12 WARP-safe volumetric cloud draw/readback proof using weather, shape-noise, and erosion-noise descriptors.

- [x] **Step 2: Add selected D3D12 raymarch execution**

Implement the smallest D3D12 renderer/RHI path that samples weather/shape/erosion textures, applies bounded raymarching, and produces readback evidence. Use quality tiers to cap primary and light steps.

Result: D3D12 now supports Texture3D sampled descriptors and buffer/Texture3D copies. The selected Win32 desktop runtime D3D12 package lane creates shape/erosion 3D textures, uploads package weather-map data, packs volumetric-cloud constants, renders the target-specific DXIL full-screen raymarch pass, reads back the target, and only reports renderer execution ready when upload/backend/draw/raymarch counters are positive and readback is non-zero. Native handle, audio, and precipitation counters remain zero.

- [x] **Step 3: Add package and renderer-execution smokes**

Add `--require-volumetric-cloud-package-evidence` with package-only counters:

- `environment_volumetric_cloud_status=ready`
- `environment_volumetric_cloud_selected_backend=d3d12`
- `environment_volumetric_cloud_weather_map_ready=1`
- `environment_volumetric_cloud_shape_noise_ready=1`
- `environment_volumetric_cloud_erosion_noise_ready=1`
- `environment_volumetric_cloud_texture_uploads=0`
- `environment_volumetric_cloud_backend_invocations=0`
- `environment_volumetric_cloud_renderer_draws=0`
- `environment_volumetric_cloud_raymarch_passes=0`
- `environment_volumetric_cloud_readback_nonzero=0`
- `environment_volumetric_cloud_native_handle_access=0`

Add `--require-volumetric-cloud-renderer-execution` with execution counters:

- `environment_volumetric_cloud_texture_uploads>0`
- `environment_volumetric_cloud_backend_invocations>0`
- `environment_volumetric_cloud_renderer_draws>0`
- `environment_volumetric_cloud_raymarch_passes>0`
- `environment_volumetric_cloud_readback_nonzero=1`
- `environment_volumetric_cloud_native_handle_access=0`
- `environment_volumetric_cloud_audio_playback=0`
- `environment_volumetric_cloud_precipitation_execution=0`

Result: cloud-layer and volumetric-cloud counters remain distinct, and the selected recipes are registered as `desktop-runtime-sample-game-volumetric-cloud-package` and `desktop-runtime-sample-game-volumetric-cloud-renderer-execution`.

- [x] **Step 4: Preserve strict Vulkan and Metal non-claims**

Add Vulkan proof only with SPIR-V validation and synchronization2 evidence. Add Metal proof only through `renderer-metal-apple-host-evidence` or a new reviewed Apple-host recipe.

Result: this candidate does not promote Vulkan or Metal volumetric-cloud readiness. Vulkan/Metal volumetric-cloud execution remains a later backend-local proof and cannot be inferred from the selected D3D12 package lane.

**2026-06-05 Task 6 Evidence:** GREEN:

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-json-contracts.ps1` passed after manifest fragment updates and compose output regeneration.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\run-validation-recipe.ps1 -Recipe desktop-runtime-sample-game-volumetric-cloud-package -Mode DryRun` passed with the expected `d3d12-windows-primary` host gate and package-only command plan.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\run-validation-recipe.ps1 -Recipe desktop-runtime-sample-game-volumetric-cloud-renderer-execution -Mode DryRun` passed with the expected `d3d12-windows-primary` host gate and renderer-execution command plan.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-ai-integration.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-public-api-boundaries.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-format.ps1` passed after formatting the touched C++/text files.
- `pwsh -NoProfile -ExecutionPolicy Bypass -Command "& { & '.\tools\check-tidy.ps1' -Files @('engine/renderer/src/volumetric_cloud_policy.cpp','engine/rhi/d3d12/src/d3d12_backend.cpp','tests/unit/d3d12_rhi_tests.cpp','tests/unit/renderer_volumetric_cloud_tests.cpp') }"` passed for the dev preset sources in the compile database; runtime-host/sample sources are covered by the desktop-runtime-release build and package smoke below.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\cmake.ps1 --preset dev` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\cmake.ps1 --build --preset dev --target MK_renderer_volumetric_cloud_tests MK_d3d12_rhi_tests` passed for the available dev targets; `sample_desktop_runtime_game` is intentionally absent from the dev preset because it requires the desktop-runtime lane.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\ctest.ps1 --preset dev --output-on-failure -R "MK_renderer_volumetric_cloud_tests|MK_d3d12_rhi_tests"` passed with 2/2 tests.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\cmake.ps1 --preset desktop-runtime-release -DMK_DESKTOP_RUNTIME_PACKAGE_GAME_TARGET=sample_desktop_runtime_game -DMK_REQUIRE_DESKTOP_RUNTIME_DXIL=ON` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\cmake.ps1 --build --preset desktop-runtime-release --target sample_desktop_runtime_game` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -Command "& { & '.\tools\package-desktop-runtime.ps1' -GameTarget 'sample_desktop_runtime_game' -RequireD3d12Shaders -SmokeArgs @('--smoke','--max-frames','2','--require-config','runtime/sample_desktop_runtime_game.config','--require-scene-package','runtime/sample_desktop_runtime_game.geindex','--require-d3d12-scene-shaders','--require-d3d12-renderer','--require-scene-gpu-bindings','--require-postprocess','--require-postprocess-depth-input','--require-d3d12-postprocess-evidence','--require-volumetric-cloud-renderer-execution') }"` passed with `installed-desktop-runtime-validation: ok (sample_desktop_runtime_game)` and `desktop-runtime-package: ok (sample_desktop_runtime_game)`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -Command "& { & '.\tools\validate-installed-desktop-runtime.ps1' -InstallPrefix '.\out\install\desktop-runtime-release' -GameTarget 'sample_desktop_runtime_game' -RequireD3d12Shaders -SmokeArgs @('--smoke','--max-frames','2','--require-config','runtime/sample_desktop_runtime_game.config','--require-scene-package','runtime/sample_desktop_runtime_game.geindex','--require-d3d12-scene-shaders','--require-d3d12-renderer','--require-scene-gpu-bindings','--require-postprocess','--require-postprocess-depth-input','--require-d3d12-postprocess-evidence','--require-volumetric-cloud-package-evidence') }"` passed and reported package-only `environment_volumetric_cloud_*` rows with zero upload/backend/draw/raymarch/readback counters.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\validate.ps1` passed after the C++/runtime/package/agent surface updates; Metal and Apple checks remain diagnostic host-gated on this Windows host.

The ready claim is limited to selected D3D12 `sample_desktop_runtime_game` package-visible volumetric-cloud evidence and selected D3D12 renderer execution evidence. Vulkan volumetric-cloud execution, Metal volumetric-cloud execution, backend parity, broad optimization, broad cloud/weather readiness, and broad `environment_ready` remain unclaimed.

## Task 7: Environment Lighting And IBL Package Evidence

**Files:**
- Modify: `engine/renderer/include/mirakana/renderer/environment_lighting_policy.hpp`
- Modify: `engine/renderer/src/environment_lighting_policy.cpp`
- Modify: `tests/unit/renderer_environment_lighting_policy_tests.cpp`
- Modify: `games/sample_desktop_runtime_game/main.cpp`
- Modify: `games/sample_desktop_runtime_game/game.agent.json`
- Modify: `games/sample_desktop_runtime_game/source/assets/package.geassets`
- Modify: `games/sample_desktop_runtime_game/runtime/sample_desktop_runtime_game.geindex`
- Create: `games/sample_desktop_runtime_game/source/textures/environment_ibl.texture_source`
- Create: `games/sample_desktop_runtime_game/runtime/assets/desktop_runtime/environment_ibl.texture.geasset`
- Modify: `tools/validate-installed-desktop-runtime.ps1`
- Modify: `tools/validation-recipe-core.ps1`
- Modify: `tools/run-validation-recipe-plans.ps1`
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `engine/agent/manifest.fragments/014-gameCodeGuidance.json`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/ai-game-development.md`
- Generate: `engine/agent/manifest.json`

- [x] **Step 1: Decide no-new-dependency versus importer-backed path**

Choose one path:

- First-party cooked texture payloads only, using existing texture/package records.
- Optional OpenEXR/KTX-class importer, with vcpkg feature, legal records, dependency docs, package tests, and license notices.

Result: Task 7 selects the first-party cooked texture payload path only. No OpenEXR/KTX/Basis importer dependency, vcpkg feature, dependency/legal document change, or third-party license notice is selected in this candidate. The selected package file is a first-party `GameEngine.CookedTexture.v1` fixture with explicit `GameEngine.FirstPartyIblMetadata.v1` rows for reflection-cubemap role, 6 faces, 16 edge size, 5 mips, `rgba16_float` IBL metadata, 9 irradiance coefficients, 5 radiance mip rows, and HDR clamp metadata.

- [x] **Step 2: Add RED IBL package tests**

Require reflection cubemap, irradiance, radiance mip rows, HDR clamp policy, package source, renderer upload evidence if ready, and dependency-gated rows if not ready.

Result: `MK_renderer_environment_lighting_policy_tests` now requires package record/source/HDR metadata fields and diagnostics before package evidence can become ready. Policy-only rows cannot claim package-ready status. `sample_desktop_runtime_game --require-environment-lighting-package-evidence` reports package-visible IBL evidence only: renderer upload evidence, texture uploads, backend invocations, runtime captures, and native-handle access all remain zero.

- [x] **Step 3: Validate**

Run focused tests plus dependency/legal checks if the importer-backed path is selected:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_environment_lighting_policy_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "environment_lighting"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
```

Expected: IBL package evidence is exact; runtime cubemap capture remains unclaimed unless a separate execution task proves it.

**2026-06-05 Task 7 Evidence:** GREEN:

- Context7 `/godotengine/godot-docs` rechecked sky radiance/reflection and irradiance/ambient-light separation plus sky bake/cubemap cache behavior. Microsoft Direct3D 12 documentation rechecked texture subresource upload and `D3D12_TEXCUBE_SRV` implications. This candidate intentionally stops at package-visible first-party IBL metadata evidence and does not claim renderer cubemap upload.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\cmake.ps1 --build --preset dev --target MK_renderer_environment_lighting_policy_tests` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\ctest.ps1 --preset dev --output-on-failure -R "environment_lighting"` passed with 1/1 tests.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\cmake.ps1 --preset desktop-runtime-release -DMK_DESKTOP_RUNTIME_PACKAGE_GAME_TARGET=sample_desktop_runtime_game -DMK_REQUIRE_DESKTOP_RUNTIME_DXIL=ON` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\cmake.ps1 --build --preset desktop-runtime-release --target sample_desktop_runtime_game` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -Command "& { & '.\tools\package-desktop-runtime.ps1' -GameTarget 'sample_desktop_runtime_game' -RequireD3d12Shaders -SmokeArgs @('--smoke','--max-frames','2','--require-config','runtime/sample_desktop_runtime_game.config','--require-scene-package','runtime/sample_desktop_runtime_game.geindex','--require-d3d12-scene-shaders','--require-d3d12-renderer','--require-scene-gpu-bindings','--require-postprocess','--require-postprocess-depth-input','--require-d3d12-postprocess-evidence','--require-environment-lighting-package-evidence') }"` passed with `installed-desktop-runtime-validation: ok (sample_desktop_runtime_game)` and `desktop-runtime-package: ok (sample_desktop_runtime_game)`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\run-validation-recipe.ps1 -Recipe desktop-runtime-sample-game-environment-lighting-package -Mode DryRun` passed with the expected `d3d12-windows-primary` host gate and package-only command plan.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-json-contracts.ps1` passed after the content mutation ledger, validation recipe, and manifest fragment updates.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-ai-integration.ps1` passed after static contract needles were updated to distinguish IBL package proof from later renderer upload/runtime-capture proof.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-public-api-boundaries.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-format.ps1` passed after formatting the touched C++ and text files.
- `pwsh -NoProfile -ExecutionPolicy Bypass -Command "& { & '.\tools\check-tidy.ps1' -Files @('engine/renderer/src/environment_lighting_policy.cpp','tests/unit/renderer_environment_lighting_policy_tests.cpp') }"` passed. `games/sample_desktop_runtime_game/main.cpp` is not present in the dev preset compile database and is covered by the desktop-runtime-release build and package smoke above.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-agents.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\validate.ps1` passed with `validate: ok`; CTest reported `100% tests passed, 0 tests failed out of 99`. Metal and Apple checks remain explicit host-gated diagnostics on this Windows host.

The ready claim is limited to selected D3D12 `sample_desktop_runtime_game` package-visible first-party IBL metadata evidence. Runtime cubemap capture, renderer/RHI cubemap upload, D3D12 texture-cube SRV execution, Vulkan IBL proof, Metal IBL proof, backend parity, broad optimization, broad lighting readiness, and broad `environment_ready` remain unclaimed.

## Task 8: Metal Host Evidence For Selected Environment Features

**Files:**
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`
- Modify: `tools/run-validation-recipe-plans.ps1`
- Modify: `tools/check-apple-host-evidence.ps1` only if the existing recipe cannot express the selected feature
- Modify: Metal backend files under the existing `MK_rhi_metal` or backend scaffold surface
- Modify: `tests/unit/backend_scaffold_tests.cpp`
- Modify: `docs/testing.md`

- [x] **Step 1: Re-open Apple Metal official docs**

Use Apple render-pass configuration, render/compute pipeline state docs, and Metal Feature Set Tables.

Expected: host gates name macOS, Xcode, `metal`, `metallib`, feature availability, and selected validation recipe ids.

Evidence: Task 8 reopened Apple official render-pass configuration, `MTLRenderPipelineState`, `MTLComputePipelineState`, `MTLDevice`, `MTLRenderCommandEncoder`, and the 2026-02-05 Metal Feature Set Tables. Apple API pages are JavaScript-rendered, while the official Feature Set Tables PDF explicitly lists GPU-family feature availability for cube map texture arrays, memory barriers, read/write cube map textures, argument/resource limits, and runtime property checks. This candidate uses those sources only to define host-gated requirements, not ready claims.

- [x] **Step 2: Add host-gated rows**

For each selected environment feature, add rows that stay `host_evidence_required` until Apple-host validation runs:

- physical sky
- height fog
- cloud layer
- precipitation
- volumetric fog
- volumetric cloud
- environment lighting/IBL

Expected: Windows and Vulkan evidence do not change Metal rows.

Result: `MetalEnvironmentFeatureHostEvidenceDesc`, `MetalEnvironmentFeatureEvidenceRequirement`, `MetalEnvironmentFeatureEvidenceRow`, `MetalEnvironmentFeatureHostEvidencePlan`, `default_environment_feature_evidence_requirements`, and `build_environment_feature_host_evidence_plan` now live in `MK_rhi_metal`. The default requirements cover physical sky, height fog, cloud layer, precipitation, volumetric fog, volumetric cloud, and environment lighting/IBL. The plan returns `host_evidence_required` on Windows or before the reviewed `renderer-metal-apple-host-evidence` recipe supplies Apple runtime, command queue, non-empty `metallib`, render/compute pipeline, render-pass, depth/particle, cube-map, and HDR texture evidence as required by each feature. Missing feature-local proof and native-handle access fail closed.

- [ ] **Step 3: Validate on Apple host**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Recipe renderer-metal-apple-host-evidence
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset ci-macos-appleclang
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset ci-macos-appleclang
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset ci-macos-appleclang --output-on-failure
```

Expected: exact feature rows can become Metal-ready only from Apple-host evidence.

Current Windows-host result: host-gated. The Windows candidate validated the value contract with `MK_backend_scaffold_tests`, `tools/check-apple-host-evidence.ps1`, recipe dry-run, manifest/static checks, and full `tools/validate.ps1`; it did not run Apple-host `-RequireReady`, `ci-macos-appleclang`, or Metal native feature execution.

**2026-06-05 Task 8 Windows Evidence:** GREEN/HOST-GATED:

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\cmake.ps1 --build --preset dev --target MK_backend_scaffold_tests` passed after adding the Metal environment host-gated value contract.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\ctest.ps1 --preset dev --output-on-failure -R "backend_scaffold"` passed; `MK_backend_scaffold_tests` verifies all seven selected environment feature rows stay `host_evidence_required` on Windows, require Apple host validation before ready, reject missing feature-local proof, and reject native-handle access.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-apple-host-evidence.ps1` exited 0 with `apple-host-evidence-check: host-gated` on this Windows host and reported missing macOS/Xcode/`xcrun`/iOS Simulator/`metal`/`metallib` blockers.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\run-validation-recipe.ps1 -Mode DryRun -Recipe renderer-metal-apple-host-evidence` passed and returned host gate `metal-apple`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-json-contracts.ps1`, `tools\check-ai-integration.ps1`, `tools\check-agents.ps1`, `tools\check-public-api-boundaries.ps1`, `tools\check-format.ps1`, and `git diff --check` passed after docs/manifest/skill/static-check synchronization.
- `pwsh -NoProfile -ExecutionPolicy Bypass -Command "& { & '.\tools\check-tidy.ps1' -Files @('engine/rhi/metal/src/metal_backend.cpp','tests/unit/backend_scaffold_tests.cpp') }"` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\validate.ps1` passed with `validate: ok`; CTest reported `100% tests passed, 0 tests failed out of 99`. Metal shader tools and Apple host evidence remain explicit host-gated diagnostics on this Windows host.

## Task 9: First-Party Editor Environment Authoring Productization

**Files:**
- Modify: `editor/core/include/mirakana/editor/environment_authoring.hpp`
- Modify: `editor/core/src/environment_authoring.cpp`
- Modify: `editor/src/native_editor_app.cpp`
- Modify: `editor/src/native_editor_app.hpp`
- Modify: `tests/unit/editor_environment_tests.cpp`
- Modify: `docs/editor.md` if present or the current editor docs surface
- Modify: `engine/agent/manifest.fragments/014-gameCodeGuidance.json`

- [ ] **Step 1: Add RED editor-core review tests**

Require retained rows for physical sky package status, fog, volumetric fog, cloud layer, volumetric clouds, rain, snow, IBL, backend evidence status, and unsupported claim diagnostics.

Expected: editor-core remains GUI-independent and no native handles are exposed.

- [ ] **Step 2: Add visible first-party panel only after core rows pass**

Use `mirakana::ui` / `MK_editor` retained UI patterns. Do not add Dear ImGui, SDL3, or UI middleware.

Expected: visible authoring is a first-party shell feature backed by the existing editor-core model.

- [ ] **Step 3: Validate**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "editor_environment"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
```

Expected: environment authoring surfaces show exact readiness rows and unsupported claims.

## Task 10: Quality Budgets, Profiling, And Closeout

**Files:**
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/testing.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md`
- Modify: `engine/agent/manifest.fragments/*.json`
- Modify: static checks under `tools/check-*.ps1` when they enforce new durable rows

- [ ] **Step 1: Add environment quality budget rows**

For each implemented feature, add package-visible budgets or diagnostics that prove bounded sample counts, particle counts, draw/dispatch counts, transient bytes, and framegraph barrier counts where applicable.

Expected: these are evidence counters, not broad optimization claims.

- [ ] **Step 2: Run full closeout validation**

Run focused recipes first, then:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1
```

Expected: full validation passes before any completion claim or PR publication.

- [ ] **Step 3: Close the plan**

Update registry, master plan, backlog, current capabilities, roadmap, manifest fragments, composed manifest, static checks, and docs so this plan is no longer active after completion.

Expected: `currentActivePlan` returns to the production-completion selection gate or a newly selected dated plan, `recommendedNextPlan` is not stale, `unsupportedProductionGaps = []` remains true unless a concrete documented blocker is discovered, and broad `environment_ready` remains absent unless every aggregate prerequisite is proven.

## Final Done When

- Every selected claim in the Claim Matrix is either implemented with exact evidence, host-gated with exact recipe and host requirements, or still explicitly unclaimed.
- D3D12, Vulkan, and Metal readiness are backend-local and feature-local.
- Rain and snow are separated; snow package readiness no longer depends on the rain lane.
- Cloud layer and volumetric cloud are separated.
- Fog and volumetric fog are separated.
- IBL package/importer path is selected cleanly and documented.
- Dear ImGui and SDL3 remain absent.
- Public game-facing APIs remain first-party, backend-neutral, and native-handle-free.
- Docs, plans, manifest fragments, composed manifest, static checks, and validation recipes agree.
- Focused validation and final `tools/validate.ps1` pass before completion.
