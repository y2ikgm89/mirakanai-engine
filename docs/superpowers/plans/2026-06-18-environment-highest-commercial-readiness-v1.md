# 2026-06-18 Environment Highest Commercial Readiness v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` or `superpowers:executing-plans` after the operator explicitly selects this plan for implementation. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `environment-highest-commercial-readiness-v1`

**Status:** Active. Tasks 1-22 select this highest-level plan as `currentActivePlan`, keep the Context7/source gate ready, add the clean-break commercial v2 gate, replace Linux/Android/iOS platform skeletons with exact host validators, replace backend parity v2 with a 15-feature / 45-row backend-local closeout, add the broad optimization artifact validator, promote the production AAA preset asset-library dependency row, replace the full OpenEXR/KTX2/Basis asset-pipeline skeleton with a fail-closed validator, replace the physical weather simulation skeleton with a fail-closed CPU/D3D12/Vulkan/Metal closeout validator, replace the production artist workflow skeleton with a 14-row package-visible closeout validator, replace the final aggregate skeleton with `tools/validate-environment-highest-commercial-readiness.ps1`, add required `validate.yml ios-metal` evidence for iOS Metal, retain Windows D3D12 typeperf optimization artifacts, retain strict Vulkan timestamp/validation optimization artifacts, add the macOS-only Apple `xcrun xctrace` Metal optimization artifact producer/upload lane, retain the PR #692 Apple-host Metal artifact output, add a hosted `validate.yml` Linux Vulkan host-evidence lane that captures `vulkaninfo --summary`, `vulkaninfo --json`, `spirv-val`, and `tools/validate-linux-vulkan-runtime-host.ps1` output without promoting Linux readiness, add the Linux XCB Vulkan surface foundation, add the first-party Linux XCB runtime host foundation, tighten Android Vulkan to official AGI GPU debug layer plus offscreen readback smoke evidence, and add the Linux package/installed validator foundation with package smoke/readback/clean-log counters. Task 13 promotes only the iOS Metal platform row and clean-break Metal aggregate row; Tasks 14-17 complete the three-backend optimization matrix and promote only `environment_broad_optimization_ready`; Task 18 records hosted Linux Vulkan evidence while keeping the Linux Vulkan platform row host-gated; Task 19 adds `SurfaceHandle::context`, `SurfacePlatform::xcb`, `VK_KHR_xcb_surface` planning, and private `vkCreateXcbSurfaceKHR` probing while keeping Linux readiness unpromoted; Task 20 adds `MK_runtime_host_linux`, `LinuxXcbWindow`, `LinuxDesktopEventPump`, and `LinuxDesktopGameHost` with private dynamic XCB loading and `NullRenderer` fallback while still keeping Linux Vulkan presentation/package readiness unpromoted; Task 21 keeps Android Vulkan host-gated until approved package/device smoke emits GPU debug layer, validation enumeration, clean logcat, and readback counters; Task 22 keeps Linux Vulkan host-gated until approved Linux package smoke emits package, Vulkan readback, and clean validation-log counters. Current evidence has 11 ready rows, 5 host-gated rows, 0 unsupported rows, 21 retained optimization workload/backend rows, and 0 missing optimization artifacts. Commercial readiness, unconditional all-platform readiness, and broad `environment_ready` remain unclaimed.

**Goal:** Promote the environment feature set to a clean-break commercial capability only when strict Vulkan, Apple Metal, backend parity, exact all-platform readiness, measured optimization, AAA preset assets, OpenEXR/KTX2/Basis production asset ingestion, physically based weather simulation, and production artist workflows all have explicit package-visible evidence.

**Architecture:** Keep authoring, import, runtime, renderer policy, RHI execution, and readiness aggregation separate. `MK_environment` owns backend-neutral environment and weather contracts, `MK_tools` owns source import/cook/package mutation, `MK_renderer` owns renderer policy and parity matrices, private `MK_rhi_*` backends own GPU execution, `MK_editor_core` owns workflow value models, and `MK_editor` owns visible shell execution. No public native OS, GPU, texture, command queue, command buffer, or third-party dependency handle is exposed.

**Tech Stack:** C++23, PowerShell 7 repository tools, CMake presets through repository wrappers, vcpkg manifest features, HLSL, DXC DXIL, DXC SPIR-V CodeGen, SPIR-V Tools, Vulkan 1.3 with synchronization2 and dynamic rendering, `VK_LAYER_KHRONOS_validation`, Direct3D 12 resource barriers/debug-layer/GPU-based-validation evidence, Apple Metal command queues/resources/pipelines/synchronization on Apple hosts, OpenEXR, KTX2/Basis Universal, glTF 2.0, OpenUSD, MaterialX, OpenColorIO, CF/netCDF/GRIB-reviewed weather datasets, first-party package/runtime validation recipes, and repository static guards.

---

## Scope Contract

This plan is the highest-level clean-break environment plan. It is wider than a single PR. Each task must land as a reviewable slice with its own tests, docs, manifest rows, and validation evidence.

The final commercial claim is:

```text
environment_highest_commercial_ready=1
environment_commercial_ready=1
environment_backend_parity_ready=1
environment_platform_readiness_ready=1
environment_all_platform_unconditional_ready=1
environment_broad_optimization_ready=1
environment_asset_pipeline_openexr_ktx_basis_full_ready=1
environment_aaa_preset_asset_library_ready=1
environment_physical_weather_simulation_ready=1
environment_artist_workflow_production_ready=1
environment_native_handle_access=0
environment_commercial_diagnostics=0
environment_host_gated_rows=0
environment_blocked_rows=0
```

The claim is valid only when all dependency rows below are ready in the same package-visible commercial closeout report:

| Dependency row | Required value | Proof boundary |
| --- | --- | --- |
| `environment_strict_vulkan_aggregate_ready` | `1` | Windows, Linux, and Android Vulkan rows each have backend-local execution, validation-layer, SPIR-V validation, synchronization2, dynamic-rendering, readback, and zero-diagnostic evidence. |
| `environment_metal_aggregate_ready` | `1` | macOS and iOS Metal rows each have Apple-host command queue, command buffer, pipeline, resource usage, synchronization/readback, feature-table/capability, and zero-diagnostic evidence. |
| `environment_backend_parity_ready` | `1` | D3D12, Vulkan, and Metal rows all satisfy the same environment feature matrix without transferring proof across APIs or hosts. |
| `environment_platform_readiness_ready` | `1` | Exact platform rows for Windows D3D12, Windows Vulkan, Linux Vulkan, macOS Metal, iOS Metal, and Android Vulkan are ready. |
| `environment_broad_optimization_ready` | `1` | Seven named workloads are measured before/after on D3D12, strict Vulkan, and Apple-host Metal, with retained profiler artifacts and no over-budget rows. |
| `environment_aaa_preset_asset_library_ready` | `1` | First-party or explicitly licensed production preset packs meet the count, provenance, package, screenshot, budget, and artist workflow criteria in Task 8. |
| `environment_asset_pipeline_openexr_ktx_basis_full_ready` | `1` | Source-to-cooked pipeline handles the declared OpenEXR and KTX2/Basis matrix, device format selection, color metadata, package replay, and runtime cooked-only ingest. |
| `environment_physical_weather_simulation_ready` | `1` | Coupled CPU reference and GPU solver rows meet validation dataset, conservation, budget, visual regression, and backend parity thresholds. |
| `environment_artist_workflow_production_ready` | `1` | Visible editor shell supports import, cook, package, preview, compare, validate, report, revise, undo/redo, batch, and review flows without editor-core backend execution. |

Commercial v2 row names are clean-break rows, not renames, aliases, compatibility bridges, or implicit promotions from earlier selected rows:

| Existing selected row | Commercial v2 row | Allowed use |
| --- | --- | --- |
| `environment_vulkan_strict_aggregate_ready` | `environment_strict_vulkan_aggregate_ready` | Historical input evidence only. Task 4 must re-emit Windows, Linux, and Android Vulkan platform rows plus the v2 aggregate in its own package-visible report. |
| `environment_metal_host_aggregate_ready` | `environment_metal_aggregate_ready` | Historical Apple-host input evidence only. Task 5 must re-emit macOS and iOS Metal platform rows plus the v2 aggregate in its own package-visible report. |
| `environment_aaa_preset_library_ready` | `environment_aaa_preset_asset_library_ready` | Selected package evidence only. Task 8 must prove the larger production asset-library count, provenance, screenshots, budgets, and workflow rows. |
| `environment_asset_pipeline_openexr_ktx_basis_ready` | `environment_asset_pipeline_openexr_ktx_basis_full_ready` | Selected source-to-package evidence only. Task 9 must prove the full declared image, compression, metadata, runtime cooked-only, replay, and backend target matrix. |
| `environment_artist_workflow_ready` | `environment_artist_workflow_production_ready` | Selected workflow evidence only. Task 11 must prove visible shell execution, revision safety, batch, compare, undo/redo, and package validation rows. |

Forbidden promotion paths:

- D3D12 evidence must not promote Vulkan, Metal, all-platform, broad optimization, physical weather, or commercial readiness.
- Windows Vulkan evidence must not promote Linux Vulkan or Android Vulkan.
- macOS Metal evidence must not promote iOS Metal.
- Earlier selected rows may seed review inputs, but every commercial v2 row must be re-emitted by its owning task with fresh package-visible counters before it can participate in the final aggregate.
- Backend parity must not promote all-platform readiness unless all exact platform rows are ready.
- Asset-library row counts must not promote AAA readiness without provenance, package, validation, screenshot, and budget rows.
- Weather visual quality review must not promote physical simulation without solver validation, conservation, and backend parity rows.
- Artist workflow value models must not promote production workflow without visible shell execution, revision safety, batch evidence, and package validation.
- No implementation may add compatibility shims, deprecated schema aliases, duplicate public APIs, or migration layers for older environment asset/profile formats.

## Official Source And Context7 Gate

Context7 is mandatory for SDK/API/dependency implementation. Task 1 originally recorded the exact refresh gate and official fallback sources while Context7 MCP tools were not exposed by tool discovery. The 2026-06-18 continuation confirmed the Context7 MCP tools are callable, and later verified the Vulkan SDK tooling, OpenUSD, MaterialX, and OpenColorIO rows. Vulkan, Vulkan SDK tooling, D3D12, OpenEXR, KTX, OpenUSD, MaterialX, OpenColorIO, and vcpkg rows are fully verified through Context7. Metal and glTF remain Context7-partial because Context7 does not expose official Apple Metal framework API docs or official Khronos glTF specification docs, but the source gate is ready because official Apple/Khronos fallback rows are recorded below. If Context7 is unavailable for a future SDK/API/dependency change, or no authoritative Context7/fallback source is recorded for the touched row, that task is blocked and must not edit implementation files.

Task 1 may still edit only plan, spec, docs, manifest fragments, composed manifest output, and the static checks that validate this active-plan/source-gate selection. Later tasks may edit task-owned C++, shader, CMake, package, editor, renderer, importer, tool, or game runtime implementation files only after their task-specific docs/Context7/fallback evidence is refreshed.

Before each task that touches SDK/API/dependency behavior, run `resolve-library-id` and `query-docs` for the exact query row, then record the resolved library id, date, and implementation implication in the task PR:

| Row | Context7 library name | Required query |
| --- | --- | --- |
| `context7.vulkan` | `Vulkan` | `Vulkan 1.3 synchronization2 dynamic rendering validation layers Vulkan Profiles SPIR-V validation strict runtime package evidence` |
| `context7.vulkan_sdk` | `Vulkan SDK` | `LunarG Vulkan SDK Linux Windows Android validation layers vulkaninfo spirv-val dxc SPIR-V setup` |
| `context7.metal` | `Apple Metal` | `Metal resource synchronization argument buffers texture usage feature set tables macOS iOS readback validation` |
| `context7.d3d12` | `Direct3D 12` | `Direct3D 12 resource barriers debug layer GPU-based validation upload heap readback heap texture state transitions` |
| `context7.openexr` | `OpenEXR` | `OpenEXR scene-linear scanline tiled multipart deep attributes channel pixel type production image pipeline` |
| `context7.ktx` | `KTX-Software` | `KTX2 Basis Universal ETC1S UASTC transcode GPU format selection metadata validation` |
| `context7.gltf` | `glTF` | `glTF 2.0 KHR_texture_basisu KTX2 runtime asset delivery PBR material texture metadata` |
| `context7.openusd` | `OpenUSD` | `OpenUSD asset structure composition variants materials volumes color asset resolver production pipeline` |
| `context7.materialx` | `MaterialX` | `MaterialX material graph exchange production look development USD renderer interoperability` |
| `context7.ocio` | `OpenColorIO` | `OpenColorIO ACES color management production VFX animation config display view transforms` |
| `context7.vcpkg` | `vcpkg` | `vcpkg manifest mode features builtin-baseline optional dependencies reproducible builds` |

Context7 verification evidence:

| Row | Status | Context7 library id | Date | Implementation implication |
| --- | --- | --- | --- | --- |
| `context7.vulkan` | verified | `/khronosgroup/vulkan-docs` | 2026-06-18 | Strict Vulkan rows must prove API-validation-layer coverage, SPIR-V validation, synchronization2 barriers, dynamic-rendering execution, readback evidence, and no cross-backend inference. |
| `context7.vulkan_sdk` | verified | `/khronosgroup/vulkan-tools` | 2026-06-18 | Vulkan SDK/tooling host gates must retain `vulkaninfo --json` or equivalent capability output, ICD/tool visibility, Vulkan Profiles JSON evidence, Android `adb` execution and pullback where applicable, and no strict package promotion without local tool evidence. |
| `context7.metal` | partial | `/dogukanveziroglu/metal-shading-language-specification` | 2026-06-18 | Context7 covers Metal shading language resource bindings and argument-buffer structures only. Apple framework API behavior must use the official Apple fallback row before Metal implementation work. |
| `context7.d3d12` | verified | `/websites/learn_microsoft_en-us_windows_win32_direct3d12` | 2026-06-18 | D3D12 rows must prove debug-layer or GPU-based-validation coverage where host-supported, explicit resource barriers, upload/readback heap evidence, texture state transitions, and no cross-backend inference. |
| `context7.openexr` | verified | `/academysoftwarefoundation/openexr` | 2026-06-18 | OpenEXR ingest must explicitly handle scanline/tiled images, channel and pixel types, metadata attributes, multipart/deep image policy, and fail-closed validation before production asset-pipeline promotion. |
| `context7.ktx` | verified | `/khronosgroup/ktx-software` | 2026-06-18 | KTX2/Basis ingest must validate the container, distinguish ETC1S and UASTC, select transcode targets from backend/device capabilities, preserve metadata rows, and keep runtime ingest cooked-only and fail-closed. |
| `context7.gltf` | partial | `/jkuhlmann/cgltf` | 2026-06-18 | Context7 exposes implementation libraries for parsing/loading/validating glTF data, but not the official Khronos glTF 2.0 and `KHR_texture_basisu` specifications. glTF implementation must use the official Khronos fallback row beside any library-specific implementation choice. |
| `context7.openusd` | verified | `/websites/openusd_release` | 2026-06-18 | OpenUSD pipeline rows must prove composition and prim-index behavior, variant/export policy, material variant editing context, asset-path resolution, and fail-closed pipeline validation before USD-backed asset workflow promotion. |
| `context7.materialx` | verified | `/academysoftwarefoundation/materialx` | 2026-06-18 | MaterialX workflow rows must prove material graph exchange, looks and assignments, target-specific shader definitions, standard surface/PBR models, texture library structure, USD/glTF interoperability intent, and fail-closed validation before look-development asset promotion. |
| `context7.ocio` | verified | `/academysoftwarefoundation/opencolorio` | 2026-06-18 | OpenColorIO rows must prove config loading, color-space-aware default views, display/view transforms, processor creation, CPU and generated GPU transform evidence, and fail-closed color-management validation before production color pipeline promotion. |
| `context7.vcpkg` | verified | `/microsoft/vcpkg` | 2026-06-18 | Optional dependencies belong in `vcpkg.json` manifest features with explicit feature selection, pinned baselines, repository bootstrap entrypoints, and no package installation from CMake configure. |

Current gate rows:

```text
environment_highest_readiness_context7_status=partial
environment_highest_readiness_context7_missing_tools=0
environment_highest_readiness_context7_verified_rows=9
environment_highest_readiness_context7_partial_rows=2
environment_highest_readiness_context7_pending_rows=0
environment_highest_readiness_official_fallback_rows=2
environment_highest_readiness_authoritative_doc_rows=11
environment_highest_readiness_authoritative_doc_ready_rows=11
environment_highest_readiness_source_gate_status=ready
environment_highest_readiness_code_edit_allowed=1
official_fallback.apple_metal_framework_api=ready
official_fallback.khronos_gltf_spec=ready
```

Official fallback evidence:

| Row | Status | Official sources | Date | Implementation implication |
| --- | --- | --- | --- | --- |
| `official_fallback.apple_metal_framework_api` | ready | Apple Developer `MTLCommandQueue`, `MTLCommandBuffer`, `MTLBlitCommandEncoder`, `MTLTextureUsage`, argument-buffer guidance, GPU feature detection, and Metal Feature Set Tables | 2026-06-18 | Metal work may start only with Apple-host command queue, command buffer, blit upload/readback, texture-usage, synchronization, feature-table, and macOS/iOS host evidence. This does not promote Metal aggregate, backend parity, all-platform, commercial, or broad environment readiness. |
| `official_fallback.khronos_gltf_spec` | ready | Khronos glTF 2.0 specification, Khronos `KHR_texture_basisu`, and KTX 2.0 specification | 2026-06-18 | glTF/KTX2/Basis work may start only with runtime-asset-delivery, PBR material texture metadata, KTX2 image, Basis Universal supercompression/transcode, and fail-closed validation evidence. This does not promote full asset-pipeline, commercial, or broad environment readiness. |

Official fallback links checked for this plan on 2026-06-18:

- Vulkan validation overview: <https://docs.vulkan.org/guide/latest/validation_overview.html>
- Vulkan Profiles: <https://docs.vulkan.org/guide/latest/vulkan_profiles.html>
- Khronos Vulkan Validation Layers: <https://github.com/KhronosGroup/Vulkan-ValidationLayers>
- LunarG Vulkan SDK Linux getting started: <https://vulkan.lunarg.com/doc/sdk/latest/linux/getting_started.html>
- Android Vulkan validation layers: <https://developer.android.com/ndk/guides/graphics/validation-layer>
- Apple Metal Feature Set Tables: <https://developer.apple.com/metal/Metal-Feature-Set-Tables.pdf>
- Apple Metal `MTLCommandQueue`: <https://developer.apple.com/documentation/metal/mtlcommandqueue>
- Apple Metal `MTLCommandBuffer`: <https://developer.apple.com/documentation/metal/mtlcommandbuffer>
- Apple Metal `MTLBlitCommandEncoder`: <https://developer.apple.com/documentation/metal/mtlblitcommandencoder>
- Apple Metal `MTLTextureUsage`: <https://developer.apple.com/documentation/metal/mtltextureusage>
- Apple Metal argument buffers: <https://developer.apple.com/documentation/Metal/managing-groups-of-resources-with-argument-buffers>
- Apple Metal CPU performance with argument buffers: <https://developer.apple.com/documentation/metal/improving-cpu-performance-by-using-argument-buffers>
- Apple Metal GPU feature detection: <https://developer.apple.com/documentation/Metal/detecting-gpu-features-and-metal-software-versions>
- Microsoft D3D12 resource barriers: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12>
- Microsoft D3D12 debug layer and GPU-based validation: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-d3d12-debug-layer-gpu-based-validation>
- Khronos glTF 2.0 specification: <https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html>
- Khronos glTF `KHR_texture_basisu` extension: <https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_texture_basisu/README.md>
- Khronos KTX 2.0 specification: <https://registry.khronos.org/KTX/specs/2.0/ktxspec.v2.html>
- Khronos KTX overview: <https://www.khronos.org/ktx/>
- OpenEXR Technical Introduction: <https://openexr.com/en/latest/TechnicalIntroduction.html>
- OpenUSD introduction: <https://openusd.org/dev/intro.html>
- MaterialX: <https://materialx.org/>
- OpenColorIO: <https://opencolorio.org/>
- CF conventions: <https://cfconventions.org/>
- NOAA Weather and Climate Toolkit: <https://www.ncei.noaa.gov/products/weather-climate-toolkit>
- ECMWF ecCodes GRIB/BUFR overview: <https://www.ecmwf.int/en/learning/training/eccodes-grib-and-bufr-data-decoding-and-encoding-software>
- vcpkg manifest mode: <https://learn.microsoft.com/en-us/vcpkg/concepts/manifest-mode>
- `vcpkg.json` reference: <https://learn.microsoft.com/en-us/vcpkg/reference/vcpkg-json>

## Current Starting Point

Repository truth before selecting this plan:

- `docs/superpowers/plans/2026-06-13-environment-commercial-excellence-v1.md` is the active milestone.
- Selected D3D12 environment aggregate evidence exists.
- Selected strict Vulkan aggregate evidence exists for the reviewed Windows Vulkan package lane.
- Selected Apple-host Metal aggregate and macOS Metal platform evidence exist only behind Apple-host recipes.
- Selected package-visible backend parity is promoted only through the existing backend parity closeout.
- Linux Vulkan, Android Vulkan, all-platform readiness, broad optimization, external marketplace preset coverage, commercial aggregate readiness, and broad `environment_ready` were unclaimed at plan selection. Later tasks promote iOS Metal, the clean-break Metal aggregate row, and broad optimization only through their exact retained evidence; commercial readiness and broad `environment_ready` remain unclaimed.
- `unsupportedProductionGaps = []` remains the 1.0 truth. This plan is post-1.0 candidate work and must not rewrite the historical MVP closure.

## File Ownership Map

Create or modify these files as the plan executes:

| Path | Owner task | Responsibility |
| --- | --- | --- |
| `docs/specs/2026-06-18-environment-highest-commercial-readiness-v1.md` | Task 1 | Product and architecture specification for the highest-level environment capability. |
| `docs/superpowers/plans/2026-06-18-environment-highest-commercial-readiness-v1.md` | All tasks | Live implementation checklist and validation evidence. |
| `docs/superpowers/plans/2026-06-13-environment-commercial-excellence-v1.md` | Task 1 | Retained preceding milestone status once this plan becomes active. |
| `docs/superpowers/plans/README.md` | Task 1, closeout | Plan registry and candidate/active selection notes. |
| `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md` | Task 1, closeout | Master-plan pointer to the active environment milestone. |
| `docs/current-capabilities.md` | Each closeout | Human-readable capability truth and non-claims. |
| `docs/roadmap.md` | Each closeout | Roadmap-level status and next slice. |
| `engine/agent/manifest.fragments/009-validationRecipes.json` | Tasks 2-12 | Machine-readable validation recipes. |
| `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` | Tasks 1-12 | Readiness rows, host gates, unsupported adjacent claims, active plan pointers. |
| `engine/agent/manifest.fragments/014-gameCodeGuidance.json` | Tasks 4-12 | Game-facing validation recipes and platform capability guidance. |
| `engine/agent/manifest.json` | Tasks 1-12 | Composed manifest output only. Do not hand-edit. |
| `schemas/engine-agent.schema.json` | Tasks 2, 12 | Schema support for new readiness rows when existing schema cannot express them. |
| `tools/check-ai-integration-010-agent-baseline.ps1` | Task 1, closeout | Static guard for recommended plan selection text. |
| `tools/check-ai-integration-020-engine-manifest.ps1` | Task 1, closeout | Static guard for recommended plan and legacy closeout exclusions. |
| `tools/check-ai-integration-030-runtime-rendering.ps1` | Task 1, closeout | Static guard for renderer/runtime active-plan selection and retained evidence boundaries. |
| `tools/check-ai-integration-094-runtime-sandbox-world-pack.ps1` | Task 1, closeout | Static guard for active-plan selection without reopening completed sandbox world pack evidence. |
| `tools/check-ai-integration-095-runtime-simulation-management-pack.ps1` | Task 1, closeout | Static guard for active-plan selection without reopening completed simulation management pack evidence. |
| `tools/check-ai-integration-096-runtime-network-replication-pack.ps1` | Task 1, closeout | Static guard for active-plan selection without reopening completed network replication pack evidence. |
| `tools/check-ai-integration-097-rendering-vfx-profiling-pack.ps1` | Task 1, closeout | Static guard for active-plan selection without reopening completed rendering/VFX profiling evidence. |
| `tools/check-ai-integration-104-mavg-runtime-lod.ps1` | Task 1, closeout | Static guard for active-plan selection without requiring MAVG closeout text in the new environment plan. |
| `tools/check-ai-integration-105-mavg-gpu-culling-indirect.ps1` | Task 1, closeout | Static guard for active-plan selection without requiring MAVG closeout text in the new environment plan. |
| `tools/check-ai-integration-106-mavg-rhi-indirect-draw.ps1` | Task 1, closeout | Static guard for active-plan selection without requiring MAVG closeout text in the new environment plan. |
| `tools/check-ai-integration-107-mavg-d3d12-indexed-indirect-draw-execution.ps1` | Task 1, closeout | Static guard for active-plan selection without requiring MAVG closeout text in the new environment plan. |
| `tools/check-ai-integration-108-mavg-vulkan-indexed-indirect-draw-execution.ps1` | Task 1, closeout | Static guard for active-plan selection without requiring MAVG closeout text in the new environment plan. |
| `tools/check-ai-integration-109-mavg-d3d12-count-buffer-indirect-execution.ps1` | Task 1, closeout | Static guard for active-plan selection without requiring MAVG closeout text in the new environment plan. |
| `tools/check-ai-integration-110-mavg-vulkan-count-buffer-indirect-execution.ps1` | Task 1, closeout | Static guard for active-plan selection without requiring MAVG closeout text in the new environment plan. |
| `tools/check-ai-integration-113-mavg-vulkan-gpu-culling-dispatch.ps1` | Task 1, closeout | Static guard for active-plan selection without requiring MAVG closeout text in the new environment plan. |
| `tools/check-ai-integration-114-mavg-vulkan-compute-generated-indirect-consumption.ps1` | Task 1, closeout | Static guard for active-plan selection without requiring MAVG closeout text in the new environment plan. |
| `tools/check-ai-integration-115-mavg-payload-byte-range-page-loader.ps1` | Task 1, closeout | Static guard for active-plan selection without requiring MAVG closeout text in the new environment plan. |
| `tools/check-ai-integration-120-mavg-resident-page-recency-eviction-order.ps1` | Task 1, closeout | Static guard for active-plan selection without requiring MAVG closeout text in the new environment plan. |
| `tools/check-ai-integration-121-mavg-streaming-upload-overlap-evidence.ps1` | Task 1, closeout | Static guard for active-plan selection without requiring MAVG closeout text in the new environment plan. |
| `tools/check-json-contracts-030-tooling-contracts.ps1` | Task 1, closeout | JSON contract guard for recommended plan selection text. |
| `tools/check-json-contracts-040-agent-surfaces.ps1` | Task 1, closeout | JSON contract guard for active-plan selection and retained evidence boundaries. |
| `tools/run-validation-recipe-plans.ps1` | Tasks 2-12 | Host-gate diagnostics and guarded execution routing. |
| `tools/check-json-contracts-035-environment-commercial-readiness.ps1` | Tasks 2-12 | Static guard for exact counters, forbidden inference, and ready-gate dependencies. |
| `tools/check-validation-recipe-runner.ps1` | Tasks 2-12 | Dry-run and host-ack coverage for new recipes. |
| `tools/validate-linux-vulkan-runtime-host.ps1` | Task 4 | Linux Vulkan runtime host gate. |
| `tools/validate-android-vulkan-runtime-host.ps1` | Task 4 | Android Vulkan device/emulator host gate. |
| `tools/validate-apple-metal-platform-host.ps1` | Task 5 | macOS/iOS Metal host and package gate. |
| `tools/validate-environment-optimization-artifacts.ps1` | Task 7 | Profiler artifact and budget verifier. |
| `tools/validate-environment-weather-physics.ps1` | Task 10 | Weather solver validation dataset and image verifier. |
| `engine/environment/include/mirakana/environment/commercial_readiness_v2.hpp` | Task 2 | Clean-break dependency graph and readiness result API. |
| `engine/environment/src/commercial_readiness_v2.cpp` | Task 2 | Dependency graph evaluation. |
| `tests/unit/environment_commercial_readiness_v2_tests.cpp` | Task 2 | Fail-closed readiness tests registered by the root CMake test target pattern. |
| `engine/renderer/include/mirakana/renderer/environment_backend_parity_v2.hpp` | Task 6 | Backend-local parity row model. |
| `engine/renderer/src/environment_backend_parity_v2.cpp` | Task 6 | Backend parity evaluation. |
| `engine/runtime_rhi/include/mirakana/runtime_rhi/environment_platform_evidence_v2.hpp` | Tasks 4-6 | Runtime package evidence API for platform rows. |
| `engine/tools/include/mirakana/tools/environment_texture_pipeline_v2.hpp` | Task 9 | OpenEXR/KTX2/Basis full pipeline cook contract. |
| `engine/tools/asset/environment_texture_pipeline_v2.cpp` | Task 9 | Optional-gated source-to-cooked implementation. |
| `editor/core/include/mirakana/editor/environment_artist_workflow_v2.hpp` | Task 11 | Production workflow value model. |
| `editor/src/environment_artist_workflow_shell_v2.cpp` | Task 11 | Visible editor shell bridge. |
| `games/sample_desktop_runtime_game/game.agent.json` | Tasks 3-12 | Recipe and package evidence declarations. |
| `vcpkg.json` | Tasks 9-11 | Optional dependency feature declarations only. |
| `docs/dependencies.md` | Tasks 9-11 | Dependency records. |
| `docs/legal-and-licensing.md` | Tasks 7, 9-11 | Dependency and asset license policy. |
| `THIRD_PARTY_NOTICES.md` | Tasks 7, 9-11 | Third-party dependency and asset notices. |

## Task 1: Select The Plan And Lock Official Sources

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-06-13-environment-commercial-excellence-v1.md`
- Modify: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `docs/roadmap.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration-010-agent-baseline.ps1`
- Modify: `tools/check-ai-integration-020-engine-manifest.ps1`
- Modify: `tools/check-ai-integration-030-runtime-rendering.ps1`
- Modify: `tools/check-ai-integration-094-runtime-sandbox-world-pack.ps1`
- Modify: `tools/check-ai-integration-095-runtime-simulation-management-pack.ps1`
- Modify: `tools/check-ai-integration-096-runtime-network-replication-pack.ps1`
- Modify: `tools/check-ai-integration-097-rendering-vfx-profiling-pack.ps1`
- Modify: `tools/check-ai-integration-104-mavg-runtime-lod.ps1`
- Modify: `tools/check-ai-integration-105-mavg-gpu-culling-indirect.ps1`
- Modify: `tools/check-ai-integration-106-mavg-rhi-indirect-draw.ps1`
- Modify: `tools/check-ai-integration-107-mavg-d3d12-indexed-indirect-draw-execution.ps1`
- Modify: `tools/check-ai-integration-108-mavg-vulkan-indexed-indirect-draw-execution.ps1`
- Modify: `tools/check-ai-integration-109-mavg-d3d12-count-buffer-indirect-execution.ps1`
- Modify: `tools/check-ai-integration-110-mavg-vulkan-count-buffer-indirect-execution.ps1`
- Modify: `tools/check-ai-integration-113-mavg-vulkan-gpu-culling-dispatch.ps1`
- Modify: `tools/check-ai-integration-114-mavg-vulkan-compute-generated-indirect-consumption.ps1`
- Modify: `tools/check-ai-integration-115-mavg-payload-byte-range-page-loader.ps1`
- Modify: `tools/check-ai-integration-120-mavg-resident-page-recency-eviction-order.ps1`
- Modify: `tools/check-ai-integration-121-mavg-streaming-upload-overlap-evidence.ps1`
- Modify: `tools/check-json-contracts-030-tooling-contracts.ps1`
- Modify: `tools/check-json-contracts-040-agent-surfaces.ps1`
- Create: `docs/specs/2026-06-18-environment-highest-commercial-readiness-v1.md`

- [x] **Step 1: Resolve Context7 documentation rows**

Run the Context7 calls listed in `Official Source And Context7 Gate`. If any future SDK/API/dependency task has neither an authoritative Context7 result nor an explicit official fallback row, keep that task limited to docs/spec/manifest selection and record:

```text
environment_highest_readiness_source_gate_status=blocked
environment_highest_readiness_authoritative_doc_ready_rows=<less-than-required>
environment_highest_readiness_code_edit_allowed=0
```

No later task may edit implementation files until the same row is updated to:

```text
environment_highest_readiness_source_gate_status=ready
environment_highest_readiness_authoritative_doc_ready_rows=11
environment_highest_readiness_code_edit_allowed=1
```

- [x] **Step 2: Create the spec with exact target counters**

Create `docs/specs/2026-06-18-environment-highest-commercial-readiness-v1.md` with the final counter list from `Scope Contract`, the official-source table, and the clean-break no-compatibility rule. The spec must contain no placeholder wording.

The spec must repeat the commercial v2 row-name policy exactly:

```text
commercial v2 rows are new rows
existing selected rows are input evidence only
no rename, alias, compatibility bridge, or implicit carry-forward is allowed
```

- [x] **Step 3: Select this plan in manifest fragments**

Set:

```json
{
  "currentActivePlan": "docs/superpowers/plans/2026-06-18-environment-highest-commercial-readiness-v1.md",
  "recommendedNextPlan": {
    "id": "environment-highest-commercial-readiness-v1",
    "title": "Environment Highest Commercial Readiness v1",
    "status": "active-milestone",
    "path": "docs/superpowers/plans/2026-06-18-environment-highest-commercial-readiness-v1.md"
  }
}
```

- [x] **Step 4: Compose and verify**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
```

Expected:

```text
check-json-contracts.ps1 exits 0
check-ai-integration.ps1 exits 0
```

## Task 2: Add Fail-Closed Commercial Readiness v2 Contract

**Files:**
- Create: `engine/environment/include/mirakana/environment/commercial_readiness_v2.hpp`
- Create: `engine/environment/src/commercial_readiness_v2.cpp`
- Create: `tests/unit/environment_commercial_readiness_v2_tests.cpp`
- Modify: `engine/environment/CMakeLists.txt`
- Modify: `CMakeLists.txt`
- Modify: `tools/check-json-contracts-035-environment-commercial-readiness.ps1`

Register the focused test as a root `CMakeLists.txt` unit-test target following the existing `MK_environment_*_tests` pattern. Do not create an `engine/environment/tests/` directory.

- [x] **Step 1: Write tests first**

Add tests for:

```text
missing_dependency_keeps_environment_highest_commercial_ready_0
all_dependencies_ready_promotes_environment_highest_commercial_ready_1
d3d12_ready_does_not_promote_vulkan_or_metal
macos_metal_ready_does_not_promote_ios_metal
backend_parity_ready_does_not_promote_all_platform_when_linux_android_ios_missing
asset_library_counts_without_license_keep_ready_0
weather_visual_quality_without_solver_validation_keeps_ready_0
artist_workflow_value_rows_without_visible_shell_keep_ready_0
native_handle_access_keeps_ready_0
diagnostics_keep_ready_0
```

- [x] **Step 2: Implement clean-break API names**

Use these public names:

```cpp
namespace mirakana {

enum class EnvironmentCommercialReadinessV2RowStatus {
    missing,
    ready,
    host_gated,
    dependency_gated,
    blocked,
    unsupported
};

struct EnvironmentCommercialReadinessV2Row {
    std::string id;
    EnvironmentCommercialReadinessV2RowStatus status;
    std::string evidence_recipe_id;
    std::string evidence_host_gate_id;
    bool native_handle_access = false;
    bool diagnostics = false;
};

struct EnvironmentCommercialReadinessV2Result {
    bool highest_commercial_ready = false;
    bool commercial_ready = false;
    std::size_t required_rows = 0;
    std::size_t ready_rows = 0;
    std::size_t host_gated_rows = 0;
    std::size_t dependency_gated_rows = 0;
    std::size_t blocked_rows = 0;
    std::size_t unsupported_rows = 0;
    bool native_handle_access = false;
    bool diagnostics = false;
};

EnvironmentCommercialReadinessV2Result evaluate_environment_commercial_readiness_v2(
    std::span<const EnvironmentCommercialReadinessV2Row> rows);

}
```

- [x] **Step 3: Validate focused tests**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_environment_commercial_readiness_v2_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_environment_commercial_readiness_v2_tests
```

Expected:

```text
100% tests passed
```

## Task 3: Add Highest-Level Validation Recipe Skeletons

**Files:**
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`
- Modify: `tools/run-validation-recipe-plans.ps1`
- Modify: `tools/check-validation-recipe-runner.ps1`
- Modify: `games/sample_desktop_runtime_game/game.agent.json`

- [x] **Step 1: Add recipe ids**

Add these recipe ids without enabling broad ready output:

```text
environment-highest-commercial-readiness-closeout
environment-platform-linux-vulkan-package
environment-platform-android-vulkan-package
environment-platform-ios-metal-package
environment-backend-parity-v2-closeout
environment-broad-optimization-cross-backend-measurement
environment-asset-pipeline-openexr-ktx-basis-full
environment-aaa-preset-asset-library-production
environment-physical-weather-simulation-closeout
environment-artist-workflow-production-closeout
```

- [x] **Step 2: Add dry-run assertions**

Each recipe must have a dry-run row with command, host gate, and ready claim set to `0`. `environment-highest-commercial-readiness-closeout` must report:

```text
environment_highest_commercial_ready=0
environment_commercial_ready=0
environment_ready_promotion_blocked_until_all_rows_ready=1
```

- [x] **Step 3: Validate recipe runner**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-validation-recipe-runner.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode DryRun -Recipe environment-highest-commercial-readiness-closeout
```

Expected:

```text
check-validation-recipe-runner.ps1 exits 0
DryRun prints environment-highest-commercial-readiness-closeout
DryRun does not execute package, GPU, or host commands
```

## Task 4: Close Strict Vulkan Platform Rows

**Files:**
- Create: `tools/validate-linux-vulkan-runtime-host.ps1`
- Create: `tools/validate-android-vulkan-runtime-host.ps1`
- Modify: `engine/runtime_rhi/include/mirakana/runtime_rhi/environment_platform_evidence_v2.hpp`
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `tools/run-validation-recipe-plans.ps1`

- [x] **Step 1: Implement Linux Vulkan host gate**

The Linux gate must require:

```text
host=linux
vulkaninfo_ready=1
VK_LAYER_KHRONOS_validation_ready=1
dxc_spirv_codegen_ready=1
spirv_val_ready=1
linux_icd_runtime_ready=1
first_party_linux_runtime_host_ready=1
linux_package_script_ready=1
linux_installed_validator_ready=1
linux_package_smoke_ready=1
linux_vulkan_readback_ready=1
linux_vulkan_validation_log_clean=1
environment_platform_linux_vulkan_ready=1
environment_platform_requires_linux_vulkan_host_evidence=0
```

- [x] **Step 2: Implement Android Vulkan host gate**

The Android gate must require:

```text
host_has_android_sdk=1
host_has_android_ndk=1
adb_device_or_emulator_ready=1
android_vulkan_profile_ready=1
android_gpu_debuggable_ready=1
android_gpu_debug_layer_settings_ready=1
android_gpu_debug_layer_app_installed=1
VK_LAYER_KHRONOS_validation_ready=1
android_package_smoke_ready=1
android_vulkan_readback_ready=1
android_vulkan_validation_layer_enumerated=1
android_vulkan_validation_log_clean=1
environment_platform_android_vulkan_ready=1
```

- [x] **Step 3: Keep Windows Vulkan separate**

Windows Vulkan remains its own strict package row. It must not set Linux or Android rows.

- [x] **Step 4: Validate dry-run and no-inference guards**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode DryRun -Recipe environment-platform-linux-vulkan-package
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode DryRun -Recipe environment-platform-android-vulkan-package
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
```

Expected:

```text
Linux and Android recipes require explicit host gates
environment_platform_windows_vulkan_ready does not appear as proof for Linux or Android rows
check-json-contracts.ps1 exits 0
```

Task 4 evidence (2026-06-18):

```text
Context7 source refresh:
- /khronosgroup/vulkan-tools: vulkaninfo --summary/--json, recognized layers, device properties, extensions, and Vulkan Profiles JSON output.
- /khronosgroup/vulkan-docs: SPIR-V shader module validation, valid SPIR-V code, Shader capability, unsupported capabilities/extensions forbidden.
- /websites/developer_android: Android Vulkan validation layers, adb GPU debug layer settings, android.hardware.vulkan.version, and android.hardware.vulkan.level declarations.

RED:
- pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_env_platform_v2_tests failed before implementation because mirakana/runtime_rhi/environment_platform_evidence_v2.hpp was missing.
- pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-validation-recipe-runner.ps1 failed before implementation because environment-platform-linux-vulkan-package still used the skeleton argv and did not include -File tools/validate-linux-vulkan-runtime-host.ps1 -RequireReady.

GREEN focused validation:
- pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_env_platform_v2_tests
- pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_rhi_environment_platform_evidence_v2_tests
- pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-validation-recipe-runner.ps1
- pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode DryRun -Recipe environment-platform-linux-vulkan-package
- pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode DryRun -Recipe environment-platform-android-vulkan-package
- pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
```

Result:

```text
environment-platform-linux-vulkan-package now routes through tools/validate-linux-vulkan-runtime-host.ps1 and requires host=linux, vulkaninfo_ready=1, VK_LAYER_KHRONOS_validation_ready=1, dxc_spirv_codegen_ready=1, spirv_val_ready=1, linux_icd_runtime_ready=1, first_party_linux_runtime_host_ready=1, linux_package_script_ready=1, linux_installed_validator_ready=1, linux_package_smoke_ready=1, linux_vulkan_readback_ready=1, linux_vulkan_validation_log_clean=1, environment_platform_linux_vulkan_ready=1, and environment_platform_requires_linux_vulkan_host_evidence=0 before a Linux row can be ready.
environment-platform-android-vulkan-package now routes through tools/validate-android-vulkan-runtime-host.ps1 and requires host_has_android_sdk=1, host_has_android_ndk=1, adb_device_or_emulator_ready=1, android_vulkan_profile_ready=1, android_gpu_debuggable_ready=1, android_gpu_debug_layer_settings_ready=1, android_gpu_debug_layer_app_installed=1, VK_LAYER_KHRONOS_validation_ready=1, android_package_smoke_ready=1, android_vulkan_readback_ready=1, android_vulkan_validation_layer_enumerated=1, android_vulkan_validation_log_clean=1, environment_platform_android_vulkan_ready=1, and environment_platform_requires_android_vulkan_host_evidence=0 before an Android row can be ready.
Windows Vulkan evidence remains separate and cannot promote Linux or Android rows. Commercial readiness, unconditional all-platform readiness, and broad environment_ready remain 0/unclaimed.
```

## Task 5: Close Apple Metal Platform Rows

**Files:**
- Create: `tools/validate-apple-metal-platform-host.ps1`
- Modify: `engine/rhi/metal/**`
- Modify: `engine/runtime_rhi/include/mirakana/runtime_rhi/environment_platform_evidence_v2.hpp`
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `tools/run-validation-recipe-plans.ps1`

- [x] **Step 1: Implement macOS Metal evidence**

The macOS row must require:

```text
host=macos
xcodebuild_ready=1
xcrun_metal_ready=1
metal_feature_set_table_checked=1
metal_command_queue_ready=1
metal_command_buffer_ready=1
metal_render_pipeline_ready=1
metal_compute_pipeline_ready=1
metal_texture_usage_rows_ready=1
metal_resource_synchronization_ready=1
metal_readback_ready=1
environment_platform_macos_metal_ready=1
```

- [x] **Step 2: Implement iOS Metal evidence**

The iOS row must require:

```text
host=macos
xcode_ios_sdk_ready=1
ios_simulator_or_device_ready=1
ios_metal_feature_set_checked=1
ios_package_smoke_ready=1
ios_metal_command_queue_ready=1
ios_metal_pipeline_ready=1
ios_metal_command_buffer_ready=1
ios_metal_readback_ready=1
environment_platform_ios_metal_ready=1
```

- [x] **Step 3: Validate host-gated behavior on non-Apple hosts**

Run on Windows or Linux:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-apple-metal-platform-host.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode DryRun -Recipe environment-platform-ios-metal-package
```

Expected:

```text
environment_platform_ios_metal_ready=0
environment_platform_requires_ios_metal_host_evidence=1
environment_all_platform_unconditional_ready=0
```

**Task 5 evidence (2026-06-18):** Context7 was queried for Metal Shading Language coverage and returned `/dogukanveziroglu/metal-shading-language-specification` for shader binding semantics; framework/API requirements still use the recorded official Apple fallback for `MTLCommandQueue`, `MTLCommandBuffer`, `MTLBlitCommandEncoder`, `MTLTextureUsage`, and Metal Feature Set Tables. TDD started with `MK_runtime_rhi_environment_platform_evidence_v2_tests` failing on missing Metal platform fields, then added exact macOS/iOS rows to `EnvironmentPlatformEvidenceV2Row` so macOS requires `host=macos`, Xcode/Metal tool readiness, feature-table, command queue/buffer, render/compute pipeline, texture usage, synchronization, and readback counters, while iOS requires `host=macos`, iOS SDK/runtime or device evidence, package smoke, app-written feature-set, command queue, compute pipeline, command buffer, and readback counters. `environment-platform-ios-metal-package` now routes through `tools/validate-apple-metal-platform-host.ps1 -Platform ios -RequireReady` and no longer uses a skeleton command. `platform/ios` now compiles `IosMetalEvidence.metal` with `xcrun metal` / `xcrun metallib` into bundled `default.metallib`; `AppDelegate.mm` loads it through `newDefaultLibrary` and writes `mirakanai_ios_metal_evidence.txt`; `tools/smoke-ios-package.ps1` reads that app data container file and fails unless required `ios_metal_*` counters are present. Local Windows evidence is intentionally host-gated: `tools/validate-apple-metal-platform-host.ps1` prints `environment_platform_ios_metal_ready=0`, `environment_platform_requires_ios_metal_host_evidence=1`, and `environment_all_platform_unconditional_ready=0`; the dry-run recipe prints the exact Apple validator argv and expected iOS counters. This closes only the Task 5 validator contract, not Apple-host ready evidence on this Windows machine.

## Task 6: Promote Backend Parity v2 Only From Backend-Local Rows

**Files:**
- Create: `engine/renderer/include/mirakana/renderer/environment_backend_parity_v2.hpp`
- Create: `engine/renderer/src/environment_backend_parity_v2.cpp`
- Create: `tests/unit/renderer_environment_backend_parity_v2_tests.cpp`
- Create: `tools/validate-environment-backend-parity-v2.ps1`
- Modify: `CMakeLists.txt`
- Modify: `engine/renderer/CMakeLists.txt`
- Modify: `tools/run-validation-recipe-plans.ps1`
- Modify: `tools/check-validation-recipe-runner.ps1`
- Modify: `tools/check-json-contracts-035-environment-commercial-readiness.ps1`

- [x] **Step 1: Define exact parity matrix**

Required feature rows:

```text
physical_sky
height_fog
volumetric_fog
volumetric_cloud
cloud_layer
rain_precipitation
snow_precipitation
material_weathering
environment_lighting_ibl
postprocess_depth_input
texture_payload_rgba8_upload
texture_payload_bc7_or_astc_upload
weather_solver_gpu
debug_profiling_policy
quality_budget
```

- [x] **Step 2: Write fail-closed tests**

Tests must prove:

```text
d3d12_vulkan_ready_metal_missing_keeps_parity_0
macos_metal_ready_ios_metal_missing_keeps_all_platform_0
ready_rows_with_native_handle_access_keep_parity_0
ready_rows_with_diagnostics_keep_parity_0
explicit_missing_row_counts_as_missing_and_keeps_parity_0
all_backend_rows_ready_promotes_backend_parity_1
```

- [x] **Step 3: Validate**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-environment-backend-parity-v2.ps1 -RequireReady
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode DryRun -Recipe environment-backend-parity-v2-closeout
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-validation-recipe-runner.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
```

Expected:

```text
100% tests passed
```

## Task 7: Add Cross-Backend Optimization Evidence

**Files:**
- Create: `tools/validate-environment-optimization-artifacts.ps1`
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `games/sample_desktop_runtime_game/game.agent.json`

- [x] **Step 1: Lock workloads**

The workload matrix is fixed:

```text
preset_pack_flythrough
storm_precipitation
dense_volumetric_fog
volumetric_cloud_sunset
snowfield_material_weathering
weather_simulation_stress
asset_library_cold_load
```

The backend matrix is fixed:

```text
d3d12
vulkan_strict
metal_apple_host
```

- [x] **Step 2: Require retained artifacts**

Each workload/backend pair must record:

```text
cpu_frame_p95_before_us
cpu_frame_p95_after_us
gpu_frame_p95_before_us
gpu_frame_p95_after_us
gpu_timestamp_ticks_per_second
memory_peak_before_bytes
memory_peak_after_bytes
upload_before_bytes
upload_after_bytes
barrier_count_before
barrier_count_after
shader_compile_or_pipeline_cache_before_ms
shader_compile_or_pipeline_cache_after_ms
stutter_frames_before
stutter_frames_after
profiler_artifact_path
trace_event_json_path
artifact_hash_sha256
```

- [x] **Step 3: Require budgets**

Promotion requires:

```text
environment_optimization_measurement_workload_rows=21
environment_optimization_measurement_backend_rows=3
environment_optimization_measurement_before_after_pairs=21
environment_optimization_measurement_profiler_artifacts=21
environment_optimization_measurement_over_budget=0
environment_broad_optimization_ready=1
```

Each workload/backend pair must also record and satisfy:

```text
cpu_frame_p95_budget_us
gpu_frame_p95_budget_us
memory_peak_budget_bytes
upload_budget_bytes
stutter_frames_budget
shader_compile_or_pipeline_cache_budget_ms
cpu_frame_p95_after_us<=cpu_frame_p95_budget_us
gpu_frame_p95_after_us<=gpu_frame_p95_budget_us
memory_peak_after_bytes<=memory_peak_budget_bytes
upload_after_bytes<=upload_budget_bytes
stutter_frames_after<=stutter_frames_budget
shader_compile_or_pipeline_cache_after_ms<=shader_compile_or_pipeline_cache_budget_ms
artifact_hash_sha256 matches ^[0-9a-f]{64}$
profiler_artifact_path starts with artifacts/environment/optimization/
trace_event_json_path starts with artifacts/environment/optimization/
```

The retained artifact root is fixed as:

```text
artifacts/environment/optimization/<task-id>/<backend>/<workload>/
```

The validator must fail if a path escapes this root, if a hash is missing, if a before/after pair is absent, or if a row is over budget. Thresholds may be revised only by editing this plan, the spec, and the validation recipe in the same PR.

- [x] **Step 4: Validate artifact checker**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-environment-optimization-artifacts.ps1 -RequireReady
```

Expected when artifacts are incomplete (`-RequireReady` prints this state, then exits non-zero):

```text
environment_broad_optimization_ready=0
environment_optimization_measurement_missing_artifacts>0
```

Expected at closeout:

```text
environment_broad_optimization_ready=1
environment_optimization_measurement_missing_artifacts=0
```

Task 7 implementation evidence:

```text
tools/validate-environment-optimization-artifacts.ps1 exists and validates the fixed 7 workload x 3 backend matrix.
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-environment-optimization-artifacts.ps1 -> exits 0 with environment_optimization_measurement_missing_artifacts=21, environment_optimization_measurement_workload_rows=0, environment_optimization_measurement_backend_rows=0, environment_optimization_measurement_profiler_artifacts=0, environment_optimization_measurement_over_budget=0, environment_broad_optimization_ready=0.
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-environment-optimization-artifacts.ps1 -RequireReady -> exits non-zero after printing the same fail-closed counters because no retained official profiler artifacts are present yet.
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-validation-recipe-runner.ps1 -> exits 0 with environment-broad-optimization-cross-backend-measurement routed through tools/validate-environment-optimization-artifacts.ps1 -RequireReady instead of a skeleton.
```

Task 7 does not promote `environment_broad_optimization_ready`. Real closeout remains blocked until all 21 retained official profiler/trace artifacts and budgets pass.

## Task 8: Build The AAA Preset Asset Library As Objective Rows

**Files:**
- Modify: `engine/environment/**`
- Modify: `engine/tools/**`
- Modify: `editor/core/**`
- Modify: `games/sample_desktop_runtime_game/game.agent.json`
- Modify: `docs/legal-and-licensing.md`
- Modify: `THIRD_PARTY_NOTICES.md`

- [x] **Step 1: Define non-subjective asset counts**

`environment_aaa_preset_asset_library_ready=1` requires:

```text
sky_atmosphere_presets>=24
volumetric_cloud_presets>=24
fog_volume_presets>=16
rain_presets>=12
snow_presets>=12
wind_presets>=12
material_weathering_presets>=24
lighting_ibl_presets>=12
weather_timeline_presets>=12
biome_environment_presets>=8
sample_scene_consumption_rows>=8
preview_screenshot_rows>=144
license_provenance_rows=all_assets
package_budget_rows=all_assets
```

- [x] **Step 2: Prefer first-party assets**

Use first-party generated or hand-authored content for default preset packs. Any third-party asset requires an entry in `THIRD_PARTY_NOTICES.md` with name, source URL, retrieved date, version or commit, copyright holder, SPDX license, modification status, and distribution target.

- [x] **Step 3: Validate**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe environment-aaa-preset-asset-library-production
```

Expected:

```text
environment_aaa_preset_asset_library_ready=1
environment_preset_asset_license_missing_rows=0
environment_preset_asset_package_budget_overages=0
```

Task 8 implementation evidence:

```text
MK_environment now exposes EnvironmentPresetAssetCategory, EnvironmentPresetAssetLibraryAssetRow, EnvironmentPresetAssetLibraryPreviewScreenshotRow, EnvironmentPresetAssetLibrarySampleSceneRow, EnvironmentPresetAssetLibraryProductionDesc, EnvironmentPresetAssetLibraryProductionResult, and evaluate_environment_preset_asset_library_production.
tools/validate-environment-aaa-preset-asset-library.ps1 builds and runs MK_environment_tests before emitting ready counters.
environment-aaa-preset-asset-library-production routes through tools/validate-environment-aaa-preset-asset-library.ps1 -RequireReady instead of a skeleton.
The ready row requires 156 first-party objective asset rows, 144 preview screenshot rows, 8 sample-scene consumption rows, complete license/provenance and package-budget rows, zero external assets, zero missing objective rows, zero backend execution, zero package-script execution, and zero native-handle access.
```

Task 8 promotes only `environment_aaa_preset_asset_library_ready=1`. It does not promote physical weather simulation, production artist workflow, all-platform readiness, broad optimization readiness, commercial readiness, or broad `environment_ready`.

## Task 9: Promote Full OpenEXR/KTX2/Basis Asset Pipeline

**Files:**
- Create: `engine/tools/include/mirakana/tools/environment_texture_pipeline_v2.hpp`
- Create: `engine/tools/asset/environment_texture_pipeline_v2.cpp`
- Create: `tools/validate-environment-asset-pipeline-full.ps1`
- Modify: `CMakeLists.txt`
- Modify: `engine/tools/asset/CMakeLists.txt`
- Modify: `tools/run-validation-recipe-plans.ps1`
- Modify: `tools/check-validation-recipe-runner.ps1`
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `engine/agent/manifest.fragments/014-gameCodeGuidance.json`
- Modify: `games/sample_desktop_runtime_game/game.agent.json`
- Modify: `tools/check-json-contracts-035-environment-commercial-readiness.ps1`
- Existing dependency files: `vcpkg.json`, `docs/dependencies.md`, `docs/legal-and-licensing.md`, `THIRD_PARTY_NOTICES.md`, `tools/bootstrap-deps.ps1`, and `tools/check-dependency-policy.ps1` already keep OpenEXR and KTX behind the `asset-importers` manifest feature; Task 9 validates that existing shape instead of changing dependency membership.

- [x] **Step 1: Define supported source matrix**

Promotion requires all rows:

```text
openexr_scanline_rgba16f_ready=1
openexr_tiled_rgba16f_ready=1
openexr_multipart_ready=1
openexr_metadata_preservation_ready=1
openexr_deep_image_rejected_with_diagnostic=1
ktx2_basis_etc1s_transcode_ready=1
ktx2_basis_uastc_transcode_ready=1
ktx2_mip_level_validation_ready=1
ktx2_color_space_metadata_ready=1
d3d12_bc7_target_ready=1
vulkan_bc7_target_ready=1
metal_astc_target_ready=1
android_vulkan_astc_target_ready=1
runtime_cooked_only_ingest_ready=1
runtime_source_parsing=0
```

- [x] **Step 2: Keep dependencies optional**

`vcpkg.json` must keep OpenEXR and KTX behind a manifest feature named `asset-importers`. `tools/bootstrap-deps.ps1` remains the only dependency installation entrypoint. CMake configure must not download, restore, or install packages.

- [x] **Step 3: Validate optional feature**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-asset-importers.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe environment-asset-pipeline-openexr-ktx-basis-full
```

Expected:

```text
environment_asset_pipeline_openexr_ktx_basis_full_ready=1
environment_asset_pipeline_runtime_source_parsing=0
environment_asset_pipeline_dependency_gated_rows=0
```

Task 9 implementation evidence (2026-06-19):

- Context7 refreshed `/academysoftwarefoundation/openexr`, `/khronosgroup/ktx-software`, and `/microsoft/vcpkg`; official source rows cover OpenEXR scanline/tiled/multipart/header-metadata/deep handling, KTX2/Basis validation and ETC1S/UASTC transcode, and vcpkg manifest features with `VCPKG_MANIFEST_INSTALL=OFF`.
- `EnvironmentTexturePipelineV2Desc` / `evaluate_environment_texture_pipeline_v2_full` require 14 package-visible host-validated rows: five OpenEXR rows, four KTX2/Basis rows, four backend target rows, and one runtime cooked-only ingest row.
- `tools/validate-environment-asset-pipeline-full.ps1` builds and runs `MK_environment_texture_pipeline_v2_tests`, runs `tools/build-asset-importers.ps1`, and emits `environment_asset_pipeline_openexr_ktx_basis_full_ready=1`, `environment_asset_pipeline_required_rows=14`, `environment_asset_pipeline_ready_rows=14`, `environment_asset_pipeline_dependency_gated_rows=0`, `environment_asset_pipeline_runtime_source_parsing=0`, `environment_asset_pipeline_runtime_optional_codec_execution=0`, `environment_asset_pipeline_cmake_configure_dependency_install=0`, `environment_ready=0`, and `environment_commercial_ready=0`.
- Each Task 9 ready row must carry a concrete source artifact id, cooked artifact id, validated package counter id, replay hash evidence, and the OpenEXR deep-image rejection diagnostic. The validator fails before `tools/build-asset-importers.ps1` when `vcpkg_installed/x64-windows/share/{spng,OpenEXR,ktx}` is missing and reports `Missing asset-importers vcpkg packages`; `tools/bootstrap-deps.ps1` is the only supported dependency bootstrap entrypoint.
- Local execute evidence currently stops at that bootstrap precondition: `tools/run-validation-recipe.ps1 -Mode Execute -Recipe environment-asset-pipeline-openexr-ktx-basis-full` builds and runs `MK_environment_texture_pipeline_v2_tests` successfully, then fails with `Missing asset-importers vcpkg packages: spng, OpenEXR, ktx`. Direct `vcpkg install` and CMake configure-time dependency install remain unsupported.
- The Task 9 recipe replaces the skeleton for `environment-asset-pipeline-openexr-ktx-basis-full`; Tasks 10-12 now replace the physical-weather, production artist workflow, and final aggregate skeletons as well. Remaining highest-commercial work is evidence, not skeleton plumbing: Linux Vulkan, Android Vulkan, iOS Metal, all-platform readiness, and 21 retained optimization artifacts still need exact proof before commercial readiness can be promoted.

Task 9 promotes only `environment_asset_pipeline_openexr_ktx_basis_full_ready=1`. It does not promote physical weather simulation, production artist workflow, all-platform readiness, broad optimization readiness, commercial readiness, final aggregate readiness, or broad `environment_ready`.

## Task 10: Close Physical Weather Simulation

**Files:**
- Modify: `engine/environment/**`
- Modify: `engine/renderer/**`
- Modify: `engine/rhi/d3d12/**`
- Modify: `engine/rhi/vulkan/**`
- Modify: `engine/rhi/metal/**`
- Create: `tools/validate-environment-weather-physics.ps1`

- [x] **Step 1: Define simulation scope**

This plan defines "complete physical weather simulation" as real-time game-environment physical coupling, not operational meteorological forecasting. Required coupled fields:

```text
wind_velocity
humidity
temperature
pressure
water_vapor
cloud_water
rain_water
snow_mass
ground_wetness
snow_accumulation
fog_density
visibility
light_extinction
```

- [x] **Step 2: Require solver evidence**

Promotion requires:

```text
cpu_reference_solver_ready=1
d3d12_gpu_solver_ready=1
vulkan_gpu_solver_ready=1
metal_gpu_solver_ready=1
solver_backend_parity_ready=1
canonical_dataset_rows>=12
canonical_image_rows>=12
mass_conservation_relative_error_max<=0.005
energy_or_stability_error_max<=0.010
negative_density_cells=0
nan_or_inf_cells=0
solver_budget_overages=0
visual_regression_failures=0
environment_physical_weather_simulation_ready=1
```

- [x] **Step 3: Validate dataset provenance**

Weather dataset import/review rows must use CF/netCDF, GRIB, or synthetic first-party validation fixtures with recorded provenance. External dataset files require license and redistribution records before becoming package artifacts.

- [x] **Step 4: Validate**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-environment-weather-physics.ps1 -RequireReady
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe environment-physical-weather-simulation-closeout
```

Expected:

```text
environment_physical_weather_simulation_ready=1
environment_weather_simulation_production_solver_ready=1
environment_weather_simulation_backend_parity_ready=1
environment_weather_simulation_validation_failures=0
```

**Task 10 evidence (2026-06-19):** Context7 was queried for Vulkan (`/khronosgroup/vulkan-docs`), Direct3D 12 (`/websites/learn_microsoft_en-us_windows_win32_direct3d12`), and Metal Shading Language (`/dogukanveziroglu/metal-shading-language-specification`), with official Apple Metal framework fallback rows retained for command queue/buffer and resource synchronization. Official CF/netCDF and WMO GRIB provenance rows bound validation dataset policy. TDD started with `MK_environment_weather_simulation_tests` failing on the missing `EnvironmentPhysicalWeather*` closeout API, then added `EnvironmentPhysicalWeatherCoupledFieldKind`, validation dataset and backend solver rows, `EnvironmentPhysicalWeatherSimulationCloseoutDesc`, `EnvironmentPhysicalWeatherSimulationCloseoutResult`, `evaluate_environment_physical_weather_simulation_closeout`, and `has_environment_physical_weather_closeout_diagnostic`. The closeout requires 13 coupled fields, 12 canonical datasets, 12 canonical images, D3D12/Vulkan/Metal solver rows, exact CPU/backend hash parity, conservation/stability thresholds, zero budget/visual/validation failures, zero backend inference, and zero native-handle access. `environment-physical-weather-simulation-closeout` now routes through `tools/validate-environment-weather-physics.ps1 -RequireReady` and emits `environment_physical_weather_simulation_ready=1`, `environment_weather_simulation_production_solver_ready=1`, `environment_weather_simulation_backend_parity_ready=1`, `environment_weather_simulation_validation_failures=0`, while keeping `environment_ready=0` and `environment_commercial_ready=0`.

Task 10 promotes only `environment_physical_weather_simulation_ready=1`. It does not promote production artist workflow, unconditional all-platform readiness, broad measured optimization readiness, commercial readiness, final aggregate readiness, or broad `environment_ready`.

## Task 11: Close Production Artist Workflow

**Files:**
- Create: `editor/core/include/mirakana/editor/environment_artist_workflow_v2.hpp`
- Create: `editor/core/src/environment_artist_workflow_v2.cpp`
- Modify: `tests/unit/editor_environment_tests.cpp`
- Reuse: `editor/src/first_party_editor_document.cpp` / `editor/src/native_editor_app.cpp` visible shell bridge rows
- Modify: `editor/src/**`
- Modify: `games/sample_desktop_runtime_game/game.agent.json`
- Create: `tools/validate-environment-artist-workflow-production.ps1`

- [x] **Step 1: Define visible workflow rows**

Promotion requires:

```text
workflow_import_openexr_ready=1
workflow_import_ktx2_basis_ready=1
workflow_import_gltf_material_ready=1
workflow_review_usd_materialx_ocio_ready=1
workflow_cook_package_ready=1
workflow_live_preview_d3d12_ready=1
workflow_live_preview_vulkan_ready=1
workflow_live_preview_metal_host_ready=1
workflow_weather_timeline_edit_ready=1
workflow_preset_batch_apply_ready=1
workflow_validation_report_ready=1
workflow_profiler_artifact_review_ready=1
workflow_undo_redo_revision_safety_ready=1
workflow_operator_review_ready=1
environment_artist_workflow_production_ready=1
```

- [x] **Step 2: Keep editor-core side effects blocked**

`MK_editor_core` may build value models, command plans, and reports. It must not execute package scripts, validation recipes, GPU commands, filesystem mutation outside reviewed document apply paths, native handles, or dependency tools.

- [x] **Step 3: Validate shell execution**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_environment_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_editor_environment_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe environment-artist-workflow-production-closeout
```

Expected:

```text
environment_artist_workflow_production_ready=1
environment_artist_workflow_editor_core_backend_execution=0
environment_artist_workflow_native_handle_access=0
```

Task 11 evidence (2026-06-19):

- Added the clean-break `EnvironmentArtistWorkflowProduction*` value model in `MK_editor_core`, with fail-closed review rows for OpenEXR import, KTX2/Basis import, glTF material import, USD/MaterialX/OCIO review, package cooking, D3D12/Vulkan/Metal-host live preview, weather timeline editing, preset batch apply, validation report review, profiler artifact review, undo/redo revision safety, and operator review.
- Added `tools/validate-environment-artist-workflow-production.ps1 -RequireReady`, which builds/runs `MK_editor_environment_tests`, packages `sample_desktop_runtime_game` with `--require-environment-artist-workflow-package`, verifies all 14 production workflow counters, and keeps editor-core backend/package-script/validation-recipe/native-handle side effects at 0.
- Task 11 promotes only `environment_artist_workflow_production_ready=1`. It does not promote commercial readiness, unconditional all-platform readiness, broad measured optimization readiness, final aggregate readiness, or broad `environment_ready`.

## Task 12: Final Commercial Aggregate Closeout

**Files:**
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `engine/agent/manifest.fragments/014-gameCodeGuidance.json`
- Modify: `engine/agent/manifest.json`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `tools/check-json-contracts-035-environment-commercial-readiness.ps1`

- [x] **Step 1: Require all dependency rows**

The closeout recipe must fail closed unless it reads:

```text
environment_strict_vulkan_aggregate_ready=1
environment_metal_aggregate_ready=1
environment_backend_parity_ready=1
environment_platform_readiness_ready=1
environment_all_platform_unconditional_ready=1
environment_broad_optimization_ready=1
environment_asset_pipeline_openexr_ktx_basis_full_ready=1
environment_aaa_preset_asset_library_ready=1
environment_physical_weather_simulation_ready=1
environment_artist_workflow_production_ready=1
environment_native_handle_access=0
environment_commercial_diagnostics=0
environment_host_gated_rows=0
environment_blocked_rows=0
```

- [x] **Step 2: Add static guard against broad false positives**

`tools/check-json-contracts-035-environment-commercial-readiness.ps1` must fail if:

```text
environment_commercial_ready=1 appears without environment_all_platform_unconditional_ready=1
environment_all_platform_unconditional_ready=1 appears without all six exact platform rows
environment_broad_optimization_ready=1 appears without 21 workload/backend measurement rows
environment_physical_weather_simulation_ready=1 appears without solver_backend_parity_ready=1
environment_artist_workflow_production_ready=1 appears without visible shell execution rows
environment_asset_pipeline_openexr_ktx_basis_full_ready=1 appears without runtime_source_parsing=0
```

- [ ] **Step 3: Run final validation**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Expected:

```text
all commands exit 0
environment_highest_commercial_ready=1
environment_commercial_ready=1
environment_ready is unchanged by this plan
```

This plan does not promote broad `environment_ready=1`. Any future broad engine readiness promotion requires a separately selected plan or architecture decision, updated static guards, and explicit non-environment subsystem evidence.

Task 12 evidence (2026-06-19):

- Replaced the final `environment-highest-commercial-readiness-closeout` skeleton with `tools/validate-environment-highest-commercial-readiness.ps1`.
- The validator builds/runs `MK_environment_commercial_readiness_v2_tests`, reads the 16 clean-break commercial v2 rows from `engine/agent/manifest.json.aiOperableProductionLoop.environmentCommercialClaimMatrix`, consumes `tools/validate-environment-optimization-artifacts.ps1` for retained optimization artifact counters, and requires zero host-gated/dependency-gated/blocked/unsupported/missing/native-handle/diagnostic rows before emitting `environment_highest_commercial_ready=1` or `environment_commercial_ready=1`.
- Static guards now require `environment_commercial_ready=1` to be paired with `environment_all_platform_unconditional_ready=1`, all six exact platform rows, 21 optimization measurement rows, physical-weather backend parity, visible-shell workflow execution, `runtime_source_parsing=0`, and `environment_ready_unchanged=1`.
- Current fail-closed evidence after Task 17 artifact retention is: `environment_highest_commercial_ready=0`, `environment_commercial_ready=0`, `environment_commercial_ready_rows=11`, `environment_host_gated_rows=5`, `environment_unsupported_rows=0`, `environment_optimization_measurement_workload_rows=21`, `environment_optimization_measurement_backend_rows=3`, `environment_optimization_measurement_profiler_artifacts=21`, `environment_optimization_measurement_missing_artifacts=0`, and `environment_broad_optimization_ready=1`.
- This closes the final aggregate gate implementation and the broad optimization dependency row, not the final commercial ready claim. Required remaining evidence is Linux Vulkan, Android Vulkan, strict Vulkan v2 aggregate commercial-row evidence, platform readiness, and all-platform readiness.

## Task 13: Promote iOS Metal And Clean-Break Metal Aggregate Only From Required Hosted Evidence

**Files:**
- Modify: `.github/workflows/validate.yml`
- Modify: `.github/workflows/ios-validate.yml`
- Modify: `tools/validate-apple-metal-platform-host.ps1`
- Modify: `tools/check-ci-matrix.ps1`
- Modify: `tools/run-validation-recipe-plans.ps1`
- Modify: `tools/check-validation-recipe-runner.ps1`
- Modify: `tools/check-json-contracts-035-environment-commercial-readiness.ps1`
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `games/sample_desktop_runtime_game/game.agent.json`
- Compose: `engine/agent/manifest.json`

**Goal:** Make iOS Metal readiness and the clean-break Metal aggregate row reviewable through required hosted CI evidence, without promoting all-platform, commercial, broad optimization, or broad `environment_ready`.

**Steps:**
- [x] Add `ios-metal` to `.github/workflows/validate.yml` as a `macos-26` job required by `pr-gate`.
- [x] Route both `ios-metal` and `ios-validate.yml` through `tools/validate-apple-metal-platform-host.ps1 -Platform ios -RequireReady`.
- [x] Require `ios_metal_command_buffer_ready=1` in the recipe, manifest, game manifest, and dry-run/static guards.
- [x] Make `tools/validate-apple-metal-platform-host.ps1` compare `ExpectedEvidenceCounters` against actual output instead of only echoing requested counters.
- [x] Promote only `environment_platform_ios_metal_ready` and `environment_metal_aggregate_ready` in the clean-break claim matrix.

**Task 13 evidence (2026-06-19):** Context7 was queried for GitHub Actions and GitHub runner images, confirming the official `needs` dependency model, conditional `if` jobs, and the hosted `macos-26` runner label. Official Apple fallback rows were refreshed for Xcode command-line tools, iOS Simulator/device execution, `MTLDevice.makeCommandQueue`, `MTLCommandBuffer`, and command-buffer completion evidence. The validator now gates iOS readiness on app-written feature-set, package smoke, command queue, compute pipeline, command buffer, and readback counters, then fails if any expected counter is absent from the actual output. `validate.yml` now includes the required `ios-metal` job in `pr-gate`, while the separate iOS workflow uses the same validator. After PR #685 exposed a hosted iOS smoke timeout with only build artifacts and no simulator-phase logs, the workflows now split the iOS Simulator build from `-SkipIosBuild` smoke validation, print available simulator devices, prefer simple iPhone simulators, preserve partial child output on timeout, and bound `simctl install`, `get_app_container`, and `launch` calls. This promotes only `environment_platform_ios_metal_ready` and `environment_metal_aggregate_ready`; `environment_commercial_ready`, `environment_all_platform_unconditional_ready`, `environment_broad_optimization_ready`, and broad `environment_ready` remain `0`.

## Task 14: Retain Windows D3D12 Typeperf Optimization Artifacts Without Broad Promotion

**Goal:** Add the first real retained optimization evidence slice for the D3D12 backend, using official Windows Performance Counter/typeperf output and the existing package-visible optimization measurement counters, while keeping `environment_broad_optimization_ready=0`.

**Steps:**
- [x] Capture `sample_desktop_runtime_game` package smoke output with `--require-environment-optimization-measurement`.
- [x] Capture official Windows Performance Counter/typeperf CSV output for the same run.
- [x] Retain per-workload evidence JSON, trace-event JSON, and SHA-256-checked profiler artifacts under `artifacts/environment/optimization/2026-06-19-d3d12-typeperf-smoke/d3d12/<workload>/`.
- [x] Keep the validator fail-closed with `environment_optimization_measurement_workload_rows=7`, `environment_optimization_measurement_backend_rows=1`, `environment_optimization_measurement_missing_artifacts=14`, and `environment_broad_optimization_ready=0`.

**Task 14 evidence (2026-06-19):** The D3D12 slice uses Microsoft `typeperf` / Windows Performance Counters as retained host profiler evidence because WPR/ETW required elevated system profiling policy on this host and PIX CLI did not complete a stable capture for the short smoke run. The retained artifacts cover the seven required workloads for the D3D12 backend only: `preset_pack_flythrough`, `storm_precipitation`, `dense_volumetric_fog`, `volumetric_cloud_sunset`, `snowfield_material_weathering`, `weather_simulation_stress`, and `asset_library_cold_load`. This reduces missing optimization artifacts from 21 to 14 but does not promote broad optimization, commercial readiness, all-platform readiness, or broad `environment_ready`.

## Task 15: Retain Strict Vulkan Timestamp/Validation Optimization Artifacts Without Broad Promotion

**Goal:** Add the second real retained optimization evidence slice for the strict Vulkan backend, using Vulkan timestamp-query and `VK_LAYER_KHRONOS_validation` evidence after fixing strict runtime VUID roots, while keeping `environment_broad_optimization_ready=0`.

**Steps:**
- [x] Fix strict Vulkan smoke VUID roots for depth>1 runtime textures, fog postprocess descriptor binding, swapchain per-image layout state, and binary semaphore reuse around present.
- [x] Validate strict Vulkan package smoke and direct smoke with no `Validation Error` or `VUID-` output.
- [x] Retain per-workload evidence JSON, trace-event JSON, and SHA-256-checked `vulkan-timestamp-validation.csv` artifacts under `artifacts/environment/optimization/2026-06-19-vulkan-strict-validation-smoke/vulkan_strict/<workload>/`.
- [x] Keep the validator fail-closed with `environment_optimization_measurement_workload_rows=14`, `environment_optimization_measurement_backend_rows=2`, `environment_optimization_measurement_missing_artifacts=7`, and `environment_broad_optimization_ready=0`.

**Task 15 evidence (2026-06-19):** Context7 was queried for official Vulkan image/view compatibility, descriptor layout matching, and binary semaphore/present synchronization requirements. The runtime now maps depth>1 shader-resource textures to Vulkan 3D images and 3D image views, binds the environment fog constants buffer for the Vulkan postprocess frame path, tracks swapchain image layouts per acquired image, and waits for the present queue before reusing binary semaphore-backed frame synchronization in strict smoke. The retained artifacts cover the seven required workloads for the strict Vulkan backend only: `preset_pack_flythrough`, `storm_precipitation`, `dense_volumetric_fog`, `volumetric_cloud_sunset`, `snowfield_material_weathering`, `weather_simulation_stress`, and `asset_library_cold_load`. This reduces missing optimization artifacts from 14 to 7 but does not promote strict Vulkan commercial aggregate readiness, broad optimization, commercial readiness, all-platform readiness, or broad `environment_ready`.

## Task 16: Add Apple Host Metal xctrace Optimization Artifact Producer Without Local Promotion

**Goal:** Add the official Apple host producer for the missing `metal_apple_host` optimization artifact rows, using `xcrun xctrace` with the Metal System Trace template and the existing Apple-host aggregate validator, while keeping Windows/local evidence, broad optimization, commercial readiness, all-platform readiness, and broad `environment_ready` fail-closed until retained-reviewed artifacts exist.

**Files:**
- Add: `tools/generate-environment-metal-optimization-artifacts.ps1`
- Modify: `.github/workflows/validate.yml`
- Modify: `tools/check-ci-matrix.ps1`
- Modify: `tools/run-validation-recipe-plans.ps1`
- Modify: `tools/check-validation-recipe-runner.ps1`
- Modify: `tools/check-json-contracts-035-environment-commercial-readiness.ps1`
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Compose: `engine/agent/manifest.json`

**Steps:**
- [x] Add a macOS-only producer that requires `xcrun xctrace`, the `Metal System Trace` template, and `tools/validate-environment-metal-host-aggregate.ps1` before writing any Metal optimization artifacts.
- [x] Emit deterministic host-gated counters on non-macOS hosts, with `environment_metal_host_optimization_artifact_ready=0`, `environment_optimization_measurement_missing_artifacts=7`, `environment_broad_optimization_ready=0`, `environment_ready=0`, and `environment_commercial_ready=0`.
- [x] Write seven `metal_apple_host` workload artifact directories only after Apple-host aggregate evidence passes, including `evidence.json`, `trace-events.json`, and exported `xctrace` table-of-contents evidence per workload.
- [x] Wire the macOS `validate.yml` lane to run the producer with `-RequireReady` and upload the generated artifacts with `actions/upload-artifact`.
- [x] Register `environment-metal-host-optimization-artifact-producer` in recipe dry-runs, manifest validation recipes, CI matrix guards, and commercial-readiness static contracts.

**Task 16 evidence (2026-06-19):** Context7 was queried for `actions/upload-artifact` and the workflow uses the pinned v7 action with explicit path, retention, compression, and `if-no-files-found` behavior. Apple `xctrace` official command-line behavior was checked for Metal System Trace recording and export. The producer keeps local Windows execution host-gated, records no fake artifacts, and requires Apple-host aggregate readiness before the seven Metal workload rows can be generated. This task adds the production path to collect the remaining optimization evidence, but does not promote `environment_broad_optimization_ready`, `environment_commercial_ready`, `environment_all_platform_unconditional_ready`, or broad `environment_ready` by itself.

## Task 17: Retain Apple Host Metal xctrace Artifacts And Promote Broad Optimization Only

**Goal:** Retain the reviewed Apple-host Metal optimization artifacts produced by PR #692 and promote only `environment_broad_optimization_ready` after the full 7 workload x 3 backend retained artifact matrix validates.

**Files:**
- Add: `artifacts/environment/optimization/2026-06-19-metal-host-xctrace-smoke/metal_apple_host/**`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `engine/agent/manifest.fragments/014-gameCodeGuidance.json`
- Modify: `tools/check-json-contracts-035-environment-commercial-readiness.ps1`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-06-18-environment-highest-commercial-readiness-v1.md`
- Compose: `engine/agent/manifest.json`

**Steps:**
- [x] Download the passing PR #692 `metal-host-optimization-artifacts` CI artifact from run `27833247179`.
- [x] Retain all seven `metal_apple_host` workload rows with `evidence.json`, `trace-events.json`, `xctrace-toc.xml`, and shared `xcrun xctrace` logs and raw-bundle CI artifact provenance under `artifacts/environment/optimization/2026-06-19-metal-host-xctrace-smoke/metal_apple_host/`.
- [x] Promote only the `environment_broad_optimization_ready` commercial-claim dependency row after `tools/validate-environment-optimization-artifacts.ps1` reports 21/21 workload/backend rows, 3 backend rows, zero missing artifacts, zero invalid hashes, zero over-budget rows, and `environment_broad_optimization_ready=1`.
- [x] Keep `environment_commercial_ready`, `environment_all_platform_unconditional_ready`, `environment_platform_readiness_ready`, `environment_strict_vulkan_aggregate_ready`, and broad `environment_ready` fail-closed.

**Task 17 evidence (2026-06-20):** The retained artifact set covers `preset_pack_flythrough`, `storm_precipitation`, `dense_volumetric_fog`, `volumetric_cloud_sunset`, `snowfield_material_weathering`, `weather_simulation_stress`, and `asset_library_cold_load` for the `metal_apple_host` backend. This completes the retained optimization matrix across D3D12, strict Vulkan, and Apple-host Metal, so broad optimization is ready only for that exact measurement contract. Commercial readiness still remains blocked by host-gated Linux Vulkan, Android Vulkan, strict Vulkan aggregate, platform readiness, and unconditional all-platform readiness.

## Task 18: Add Hosted Linux Vulkan Evidence Without Promotion

**Goal:** Add a required hosted Ubuntu evidence lane for the Linux Vulkan host gate so every runtime/CI PR captures the actual Linux Vulkan tool/runtime state before Linux readiness is promoted.

**Context:** Context7 `/khronosgroup/vulkan-tools` confirms `vulkaninfo --summary` reports instance/GPU summaries and `vulkaninfo --json` emits Vulkan Profiles-schema JSON. Context7 `/websites/github_en_actions` confirms `needs` gate jobs can run with `always()`-style conditions and must explicitly evaluate skipped/failed dependencies. Ubuntu noble package pages confirm `vulkan-tools`, `vulkan-validationlayers`, `mesa-vulkan-drivers`, and `spirv-tools` are available from Ubuntu package repositories for hosted Linux diagnostics.

**Constraints:**
- The lane must not pass `-RequireReady` until `engine/runtime_host/linux`, Linux package smoke, installed package validation, strict Vulkan readback counters, and DXC SPIR-V CodeGen evidence exist.
- The lane must upload host evidence artifacts, not infer Linux readiness from Windows Vulkan or compile-only evidence.
- `environment_platform_linux_vulkan_ready`, `environment_all_platform_unconditional_ready`, `environment_commercial_ready`, and broad `environment_ready` stay `0`.

**Files:**
- Modify: `.github/workflows/validate.yml`
- Modify: `tools/check-ci-matrix.ps1`
- Modify: `docs/superpowers/plans/2026-06-18-environment-highest-commercial-readiness-v1.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Compose: `engine/agent/manifest.json`

**Steps:**
- [x] Add `linux-vulkan` to `validate.yml` as a required PR gate dependency whenever the Linux tier is selected.
- [x] Install hosted Ubuntu Vulkan diagnostic packages for `vulkaninfo`, validation layer manifests, Mesa ICD/runtime evidence, and `spirv-val`.
- [x] Retain `vulkaninfo --summary`, `vulkaninfo --json`, and `tools/validate-linux-vulkan-runtime-host.ps1` output under `linux-vulkan-host-evidence`.
- [x] Extend `tools/check-ci-matrix.ps1` to guard the job id, pinned checkout/artifact actions, upload retention, required package names, validator command, and PR gate membership.
- [x] Keep Linux Vulkan readiness host-gated until the Linux runtime/package/readback candidate lands.

**Task 18 evidence (2026-06-20):** The hosted Linux Vulkan evidence lane records real Ubuntu runner Vulkan tooling and validator output but intentionally does not promote `environment_platform_linux_vulkan_ready`. This reduces ambiguity for the next Linux runtime/package candidate while preserving the commercial gate blockers: Linux Vulkan, Android Vulkan, strict Vulkan aggregate, platform readiness, and unconditional all-platform readiness.

## Task 19: Add Linux XCB Vulkan Surface Foundation Without Promotion

**Goal:** Add the RHI-level Linux XCB surface foundation that the future `engine/runtime_host/linux` lane will need, without exposing native handles or promoting Linux readiness.

**Context:** Context7 `/khronosgroup/vulkan-docs` confirms `VK_KHR_surface` is the foundation instance extension for `VkSurfaceKHR`, `VK_KHR_xcb_surface` creates a `VkSurfaceKHR` from an XCB `connection` and `window`, and `vkCreateXcbSurfaceKHR` is the platform-specific creation entrypoint. The engine keeps native XCB/Vulkan details private to the Vulkan backend and passes only opaque integer tokens through public RHI data.

**Constraints:**
- Do not expose public XCB, Vulkan instance/device/surface/swapchain, or runtime-host native handles.
- Do not implement Linux window creation, event pump, packaging, installed validation, or package smoke in this task.
- Keep `environment_platform_linux_vulkan_ready`, `environment_platform_readiness_ready`, `environment_all_platform_unconditional_ready`, `environment_commercial_ready`, and broad `environment_ready` at `0`.

**Files:**
- Modify: `engine/rhi/include/mirakana/rhi/rhi.hpp`
- Modify: `engine/rhi/vulkan/src/vulkan_backend.cpp`
- Modify: `tests/unit/backend_scaffold_tests.cpp`
- Modify: `docs/superpowers/plans/2026-06-18-environment-linux-vulkan-platform-host-v1.md`
- Modify: `docs/superpowers/plans/2026-06-18-environment-highest-commercial-readiness-v1.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.fragments/014-gameCodeGuidance.json`
- Compose: `engine/agent/manifest.json`

**Steps:**
- [x] Add a backend-neutral `SurfacePlatform` tag and `SurfaceHandle::context` so Linux XCB can carry a connection token plus window token without native type exposure.
- [x] Add Linux `VK_KHR_surface` + `VK_KHR_xcb_surface` instance-extension planning.
- [x] Add private Linux `vkCreateXcbSurfaceKHR` probing and fail-closed diagnostics for missing XCB platform/context/window evidence.
- [x] Keep all Linux/platform/commercial aggregate rows host-gated or unclaimed.

**Task 19 evidence (2026-06-20):** The focused backend scaffold test now proves Linux extension planning includes `VK_KHR_xcb_surface` and that Linux surface support probing fails closed when XCB connection/window evidence is incomplete. This is only RHI foundation for the next Linux runtime-host/package slice; it does not create a Linux host, package script, installed validator, strict package smoke, or Linux platform readiness.

## Task 20: Add Linux XCB Runtime Host Foundation Without Promotion

**Goal:** Add the first-party Linux desktop/runtime host foundation needed before Linux Vulkan presentation and Linux package validation can be built, while preserving the host-gated Linux readiness row.

**Context:** Official XCB documentation defines the connection/window/event-polling model through `xcb_connect`, `xcb_create_window`, `xcb_poll_for_event`, and caller-owned event memory. Context7 `/kitware/cmake` confirms target-based include/link/install patterns for conditional targets. The engine keeps XCB private to `MK_runtime_host_linux`, exposes only first-party `IWindow`, `IDesktopEventPump`, `DesktopGameRunner`, and readiness value rows, and avoids new build-time XCB/pkg-config dependencies by dynamically loading the host system `libxcb.so.1`.

**Constraints:**
- Do not add SDL3, GLFW, Qt, or any window middleware.
- Do not expose public XCB connection/window, Vulkan surface, Vulkan instance/device, swapchain, or native handles.
- Do not implement Linux Vulkan presentation, package script, installed validator, strict package smoke, or Linux ready promotion in this task.
- Keep `environment_platform_linux_vulkan_ready`, `environment_platform_readiness_ready`, `environment_all_platform_unconditional_ready`, `environment_commercial_ready`, and broad `environment_ready` at `0`.

**Files:**
- Modify: `CMakeLists.txt`
- Modify: `engine/runtime_host/CMakeLists.txt`
- Add: `engine/runtime_host/include/mirakana/runtime_host/linux/linux_desktop_game_host.hpp`
- Add: `engine/runtime_host/src/linux_desktop_host_contract.cpp`
- Add: `engine/runtime_host/linux/CMakeLists.txt`
- Add: `engine/runtime_host/linux/src/linux_desktop_game_host.cpp`
- Add: `tests/unit/runtime_host_linux_public_api_compile.cpp`
- Modify: `tests/unit/runtime_host_tests.cpp`
- Modify: `tools/validate-linux-vulkan-runtime-host.ps1`
- Modify: docs, plan registry, manifest fragments, and composed manifest.

**Steps:**
- [x] Add cross-platform value-only Linux desktop host request/readiness contract and status labels.
- [x] Add Linux-only `MK_runtime_host_linux` target and Linux public API compile test.
- [x] Implement private dynamic XCB loading, XCB window creation/destruction, non-blocking event polling, resize/move/apply placement, and lifecycle close propagation.
- [x] Add `LinuxDesktopGameHost` over `DesktopGameRunner`, virtual inputs/lifecycle, and `NullRenderer` fallback only.
- [x] Update the Linux host validator to look for `engine/runtime_host/linux` and the public Linux runtime host header while still requiring Linux package script and installed validator before ready.
- [x] Keep all Linux/platform/commercial aggregate rows host-gated or unclaimed.

**Task 20 evidence (2026-06-20):** `MK_runtime_host_tests` now proves the Linux host contract stays value-only, rejects invalid requests, reports non-Linux host gating, and exposes no native handles. `tools/validate-linux-vulkan-runtime-host.ps1` still reports `environment_platform_linux_vulkan_ready=0` on Windows and will only advance the first-party host counter on Linux when the new `engine/runtime_host/linux` target/header exist; package script, installed validator, strict Linux package smoke, and readback counters remain missing blockers.

## Task 21: Tighten Android Vulkan GPU Debug Layer And Readback Smoke Without Promotion

**Goal:** Replace the stale packaged-validation-layer assumption with the official Android AGI GPU debug layer settings contract and add app-written offscreen Vulkan readback/logcat counters, while preserving the host-gated Android Vulkan row until approved package/device smoke evidence exists.

**Context:** Context7 `/websites/developer_android` confirms AGI Vulkan validation setup uses a debuggable app plus `adb shell settings put global enable_gpu_debug_layers 1`, `gpu_debug_app`, `gpu_debug_layer_app`, and `gpu_debug_layers VK_LAYER_KHRONOS_validation`. Context7 `/khronosgroup/vulkan-docs` confirms CPU readback of GPU-written data requires a pipeline barrier to host-read access, queue/fence or device-idle synchronization, and `vkInvalidateMappedMemoryRanges` for non-coherent mapped memory before host reads.

**Constraints:**
- Do not mark `environment_platform_android_vulkan_ready`, `environment_platform_readiness_ready`, `environment_all_platform_unconditional_ready`, `environment_commercial_ready`, or broad `environment_ready` ready in this task.
- Do not infer Android Vulkan readiness from desktop Vulkan, Windows Vulkan, Linux Vulkan, D3D12, package-template-only checks, or compile-only Android evidence.
- Keep the Android evidence path first-party GameActivity/Vulkan only; do not add SDL3, GLFW, Qt, or UI/window middleware.

**Files:**
- Modify: `engine/runtime_rhi/include/mirakana/runtime_rhi/environment_platform_evidence_v2.hpp`
- Modify: `tests/unit/runtime_rhi_environment_platform_evidence_v2_tests.cpp`
- Add: `platform/android/app/src/main/cpp/android_vulkan_readback_smoke.hpp`
- Add: `platform/android/app/src/main/cpp/android_vulkan_readback_smoke.cpp`
- Modify: `platform/android/app/src/main/cpp/CMakeLists.txt`
- Modify: `platform/android/app/src/main/cpp/game_activity_bridge.cpp`
- Modify: `platform/android/app/build.gradle.kts`
- Modify: `tools/validate-android-vulkan-runtime-host.ps1`
- Modify: `tools/smoke-android-package.ps1`
- Modify: `tools/check-mobile-packaging.ps1`
- Modify: validation recipes, static checks, docs, plan registry, manifest fragments, and composed manifest.

**Steps:**
- [x] Replace `android_validation_layer_packaged` with `android_gpu_debuggable_ready`, `android_gpu_debug_layer_settings_ready`, and `android_gpu_debug_layer_app_installed`.
- [x] Add offscreen Android Vulkan readback smoke that enumerates `VK_LAYER_KHRONOS_validation`, writes package-visible readback counters, uses explicit GPU-to-host synchronization, and keeps validation logs clean.
- [x] Route the Android host validator through official GPU debug layer settings, installed AGI validation-layer APK evidence, package smoke, same-launch validation-layer enumeration, clean logcat, and readback counters.
- [x] Keep Android/platform/commercial aggregate rows host-gated or unclaimed until approved device/emulator smoke produces every required ready counter.

**Task 21 evidence (2026-06-20):** TDD RED first failed `MK_env_platform_v2_tests` because `EnvironmentPlatformEvidenceV2Row` did not expose `android_gpu_debuggable_ready` or `android_gpu_debug_layer_settings_ready`. GREEN will require the focused platform-evidence test, Android host validator fail-closed output, recipe/static checks, and full validation evidence before this task can be published.

## Task 22: Add Linux Package And Installed Validator Foundation Without Promotion

**Goal:** Add the Linux-specific package script, installed package validator, and package smoke/readback/clean-log counters required before Linux Vulkan can leave host-gated state, while preserving `environment_platform_linux_vulkan_ready=0` until the Linux host package smoke truly emits every counter.

**Context:** Context7 `/kitware/cmake` confirms `cmake --install --prefix` is the official way to override the install prefix for a single install invocation, while CPack presets remain separate package invocations. Context7 `/khronosgroup/vulkan-docs` keeps the Linux Vulkan proof scoped to validation-layer and synchronized readback evidence rather than compile-only or Windows Vulkan inference.

**Constraints:**
- Do not mark `environment_platform_linux_vulkan_ready`, `environment_strict_vulkan_aggregate_ready`, `environment_platform_readiness_ready`, `environment_all_platform_unconditional_ready`, `environment_commercial_ready`, or broad `environment_ready` ready in this task.
- Do not extend the Win32 `tools/package-desktop-runtime.ps1` lane into Linux proof; Linux must use a Linux-specific script and installed validator.
- Do not accept package script existence, compile-only Linux CMake, hosted `vulkaninfo` output, or Win32 strict Vulkan evidence as Linux Vulkan readiness.

**Files:**
- Add: `tools/package-linux-runtime.ps1`
- Add: `tools/validate-installed-linux-runtime.ps1`
- Modify: `tools/validate-linux-vulkan-runtime-host.ps1`
- Modify: `engine/runtime_rhi/include/mirakana/runtime_rhi/environment_platform_evidence_v2.hpp`
- Modify: `tests/unit/runtime_rhi_environment_platform_evidence_v2_tests.cpp`
- Modify: validation recipes, static checks, docs, plan registry, manifest fragments, game manifest, and composed manifest.

**Steps:**
- [x] Add package-visible Linux counters `linux_package_smoke_ready`, `linux_vulkan_readback_ready`, and `linux_vulkan_validation_log_clean`.
- [x] Add Linux-only package and installed validators that fail closed on non-Linux hosts and require installed status-line counters before reporting success.
- [x] Update `tools/validate-linux-vulkan-runtime-host.ps1` so `-RequireReady` runs the Linux package path only after prerequisites are ready and never promotes Linux readiness from script existence alone.
- [x] Keep commercial and all-platform aggregate rows host-gated until Linux and Android exact package evidence exists.

**Task 22 evidence (2026-06-20):** TDD RED first failed `tools/check-json-contracts.ps1` because `environment-platform-linux-vulkan-package` lacked `linux_package_smoke_ready=1`; the C++ RED failed `MK_env_platform_v2_tests` because `EnvironmentPlatformEvidenceV2Row` lacked the new Linux package smoke/readback/clean-log fields. GREEN will require focused platform evidence, Linux host validator fail-closed output, recipe/static checks, and full validation before publication.

## Task 23: Add Host-Aware Vcpkg And Linux Runtime Package Preset Foundation Without Promotion

**Goal:** Make the Linux runtime package lane use host-aware vcpkg bootstrap/executable resolution and a Linux-only CMake preset/triplet instead of reusing the Windows `desktop-runtime-release` preset, while preserving `environment_platform_linux_vulkan_ready=0` until real Linux package smoke evidence exists.

**Context:** Context7 `/kitware/cmake` confirms CMake presets may define `generator`, `binaryDir`, and `cacheVariables` such as `CMAKE_BUILD_TYPE`. Context7 `/microsoft/vcpkg` confirms manifest-mode triplet selection is driven by `VCPKG_TARGET_TRIPLET` and that `VCPKG_MANIFEST_INSTALL=OFF` means dependencies are installed manually. Microsoft Learn confirms Windows vcpkg bootstrap uses `bootstrap-vcpkg.bat`, Unix uses `./bootstrap-vcpkg.sh`, and CI should call repository-owned `vcpkg` / `bootstrap-vcpkg` with a path prefix to avoid stale host-installed copies.

**Constraints:**
- Do not mark `environment_platform_linux_vulkan_ready`, `environment_strict_vulkan_aggregate_ready`, `environment_platform_readiness_ready`, `environment_all_platform_unconditional_ready`, `environment_commercial_ready`, or broad `environment_ready` ready in this task.
- Do not reuse the Windows `desktop-runtime-release` / `x64-windows` preset as Linux proof.
- Keep vcpkg installation in `tools/bootstrap-deps.ps1`; CMake configure must keep `VCPKG_MANIFEST_INSTALL=OFF`.

**Files:**
- Modify: `tools/common.ps1`
- Modify: `tools/bootstrap-deps.ps1`
- Modify: `CMakePresets.json`
- Modify: `tools/package-linux-runtime.ps1`
- Modify: validation/static checks, docs, plan registry, manifest fragments, and composed manifest.

**Steps:**
- [x] Add host-aware vcpkg executable and bootstrap script helpers that resolve `vcpkg.exe` / `bootstrap-vcpkg.bat` on Windows and `vcpkg` / `bootstrap-vcpkg.sh` on Unix-like hosts.
- [x] Route `tools/bootstrap-deps.ps1` through the host-aware bootstrap helper while preserving the single supported dependency bootstrap entrypoint.
- [x] Add the Linux-only `desktop-runtime-linux-release` configure/build/test/package presets with `generator=Ninja`, `CMAKE_BUILD_TYPE=Release`, `VCPKG_TARGET_TRIPLET=x64-linux`, and `VCPKG_MANIFEST_INSTALL=OFF`.
- [x] Route `tools/package-linux-runtime.ps1` through `desktop-runtime-linux-release`, `out/build/desktop-runtime-linux-release`, `out/install/linux-runtime-release`, and `tools/validate-installed-linux-runtime.ps1`.
- [x] Keep Linux/package/commercial aggregate rows host-gated until Linux presentation/package smoke emits every required counter on a Linux host.

**Task 23 evidence (2026-06-20):** TDD RED first failed `tools/check-json-contracts.ps1` because `CMakePresets.json` lacked `desktop-runtime-linux-release`. The first GREEN pass added host-aware vcpkg helpers, host-specific bootstrap, a Linux-only CMake preset, and Linux package-script preset routing. Publication requires static/agent-surface checks and full validation before this candidate can merge.

## Execution Order

Use this PR order:

1. Task 1 only: plan selection and official-source lock.
2. Tasks 2-3: fail-closed contract and recipe skeletons.
3. Task 4 Linux Vulkan.
4. Task 4 Android Vulkan.
5. Task 5 macOS/iOS Metal.
6. Task 6 backend parity v2.
7. Task 7 optimization measurement.
8. Task 8 preset asset library.
9. Task 9 full OpenEXR/KTX2/Basis asset pipeline.
10. Task 10 physical weather simulation.
11. Task 11 production artist workflow.
12. Task 12 commercial aggregate closeout.
13. Task 13 iOS Metal hosted CI evidence and clean-break Metal aggregate row.
14. Task 14 D3D12 typeperf optimization artifacts.
15. Task 15 strict Vulkan timestamp/validation optimization artifacts.
16. Task 16 Metal host xcrun xctrace optimization artifact producer and upload lane.
17. Task 17 retained Apple-host Metal xctrace artifacts and broad optimization dependency-row promotion.
18. Task 18 hosted Linux Vulkan evidence lane without Linux readiness promotion.
19. Task 19 Linux XCB Vulkan surface foundation without Linux readiness promotion.
20. Task 20 Linux XCB runtime host foundation without Linux readiness promotion.
21. Task 21 Android Vulkan GPU debug layer and offscreen readback smoke without Android readiness promotion.
22. Task 22 Linux package and installed validator foundation without Linux readiness promotion.
23. Task 23 host-aware vcpkg bootstrap and Linux runtime package preset foundation without Linux readiness promotion.

Do not merge Task 23 until its focused local validation, publication preflight, and task-owned PR checks pass for the task-owned PR head SHA. The final commercial row remains unready until Linux Vulkan, Android Vulkan, strict Vulkan aggregate, platform readiness, and all-platform readiness close with exact host evidence.

## Validation Evidence

Record validation here as each PR lands.

| Date | Task | Command | Result | Evidence |
| --- | --- | --- | --- | --- |
| 2026-06-18 | Plan creation | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1` | pass | `text-format-check: ok` |
| 2026-06-18 | Plan creation | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | pass | `json-contract-check: ok` |
| 2026-06-18 | Plan creation | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | pass | `ai-integration-check: ok` |
| 2026-06-18 | Plan creation | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | pass | `agent-config-check: ok` |
| 2026-06-18 | Task 1 active-plan selection | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | pass | `agent-manifest-compose: ok`; `json-contract-check: ok` |
| 2026-06-18 | Task 1 active-plan selection | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1` | pass | `text-format-check: ok` |
| 2026-06-18 | Task 1 active-plan selection | `git diff --check` | pass | no whitespace errors |
| 2026-06-18 | Task 1 active-plan selection | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | pass | `agent-config-check: ok` |
| 2026-06-18 | Task 1 active-plan selection | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | pass | `ai-integration-check: ok` |
| 2026-06-18 | Task 1 active-plan selection | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 -StaticOnly -StaticJobs 1 -StaticCheckTimeoutSeconds 120` | pass | `validate: static ok`; Apple/Metal checks remain host-gated or diagnostic-only on Windows |
| 2026-06-18 | Task 2 plan alignment | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1` | pass | `text-format-check: ok` |
| 2026-06-18 | Task 2 plan alignment | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | pass | `agent-config-check: ok` |
| 2026-06-18 | Task 2 plan alignment | `git diff --check` | pass | no whitespace errors |
| 2026-06-18 | Task 2 plan alignment | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | pass | `agent-manifest-compose: ok`; `json-contract-check: ok` |
| 2026-06-18 | Task 2 plan alignment | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | pass | `ai-integration-check: ok` |
| 2026-06-18 | Task 1 Context7 gate continuation | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1` | pass | `text-format-check: ok` |
| 2026-06-18 | Task 1 Context7 gate continuation | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | pass | `agent-manifest-compose: ok`; `json-contract-check: ok` |
| 2026-06-18 | Task 1 Context7 gate continuation | `git diff --check` | pass | no whitespace errors |
| 2026-06-18 | Task 1 Context7 gate continuation | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | pass | `agent-config-check: ok` |
| 2026-06-18 | Task 1 Context7 gate continuation | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | pass | `ai-integration-check: ok` |
| 2026-06-18 | Task 1 Context7 gate continuation | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 -StaticOnly -StaticJobs 1 -StaticCheckTimeoutSeconds 120` | pass | `validate: static ok`; Apple/Metal checks remain host-gated or diagnostic-only on Windows |
| 2026-06-18 | Task 1 MaterialX/OpenColorIO Context7 gate continuation | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1` | pass | `text-format-check: ok` |
| 2026-06-18 | Task 1 MaterialX/OpenColorIO Context7 gate continuation | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | pass | `agent-manifest-compose: ok`; `json-contract-check: ok` |
| 2026-06-18 | Task 1 MaterialX/OpenColorIO Context7 gate continuation | `git diff --check` | pass | no whitespace errors |
| 2026-06-18 | Task 1 MaterialX/OpenColorIO Context7 gate continuation | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | pass | `agent-config-check: ok` |
| 2026-06-18 | Task 1 MaterialX/OpenColorIO Context7 gate continuation | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | pass | `ai-integration-check: ok` |
| 2026-06-18 | Task 1 MaterialX/OpenColorIO Context7 gate continuation | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 -StaticOnly -StaticJobs 1 -StaticCheckTimeoutSeconds 120` | pass | `validate: static ok`; Apple/Metal checks remain host-gated or diagnostic-only on Windows |
| 2026-06-18 | Task 1 official fallback source gate | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` | pass | composed `engine/agent/manifest.json` from fragments |
| 2026-06-18 | Task 1 official fallback source gate | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | pass | `agent-manifest-compose: ok`; `json-contract-check: ok` |
| 2026-06-18 | Task 1 official fallback source gate | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1` | pass | `text-format-check: ok` |
| 2026-06-18 | Task 1 official fallback source gate | `git diff --check` | pass | no whitespace errors |
| 2026-06-18 | Task 1 official fallback source gate | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | pass | `agent-config-check: ok` |
| 2026-06-18 | Task 1 official fallback source gate | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | pass | `ai-integration-check: ok` |
| 2026-06-18 | Task 1 official fallback source gate | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 -StaticOnly -StaticJobs 1 -StaticCheckTimeoutSeconds 120` | pass | `validate: static ok`; Metal/Apple checks remain host-gated or diagnostic-only on Windows |
| 2026-06-18 | Task 2 commercial readiness v2 TDD RED | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_environment_commercial_readiness_v2_tests` | expected fail | `commercial_readiness_v2.hpp`: no such file |
| 2026-06-18 | Task 2 commercial readiness v2 focused build | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_environment_commercial_readiness_v2_tests` | pass | built `MK_environment_commercial_readiness_v2_tests` |
| 2026-06-18 | Task 2 commercial readiness v2 focused test | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_environment_commercial_readiness_v2_tests` | pass | `100% tests passed, 0 tests failed out of 1` |
| 2026-06-18 | Task 2 duplicate row review fix RED | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_environment_commercial_readiness_v2_tests` | expected fail | `duplicate_required_row_id_keeps_ready_0`: `!result.highest_commercial_ready` |
| 2026-06-18 | Task 2 duplicate row review fix GREEN | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_environment_commercial_readiness_v2_tests` | pass | `100% tests passed, 0 tests failed out of 1` |
| 2026-06-18 | Task 2 commercial readiness v2 focused tidy | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/environment/src/commercial_readiness_v2.cpp,tests/unit/environment_commercial_readiness_v2_tests.cpp` | pass | `tidy-check: ok (2 files)` |
| 2026-06-18 | Task 2 commercial readiness v2 full validation | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | pass | `validate: ok`; Apple/Metal checks remain host-gated or diagnostic-only on Windows |
| 2026-06-18 | Task 2 commercial readiness v2 publication preflight | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1` | pass | `publication-preflight: ok` |
| 2026-06-18 | Task 3 validation recipe skeletons RED | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-validation-recipe-runner.ps1` | expected fail | `environment-highest-commercial-readiness-closeout exited 2, expected 0` |
| 2026-06-18 | Task 3 validation recipe skeletons RED | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | expected fail | missing `environment-highest-commercial-readiness-closeout` validation recipe |
| 2026-06-18 | Task 3 validation recipe skeletons GREEN | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-validation-recipe-runner.ps1` | pass | `validation-recipe-runner-check: ok` |
| 2026-06-18 | Task 3 validation recipe skeletons dry-run | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode DryRun -Recipe environment-highest-commercial-readiness-closeout` | pass | prints `environment-highest-commercial-readiness-closeout`, `environment_highest_commercial_ready=0`, `environment_commercial_ready=0`, and `environment_ready_promotion_blocked_until_all_rows_ready=1`; no package, GPU, or host command executed |
| 2026-06-18 | Task 3 validation recipe skeletons JSON contract | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | pass | `agent-manifest-compose: ok`; `json-contract-check: ok` |
| 2026-06-19 | Task 12 final aggregate validator fail-closed | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-environment-highest-commercial-readiness.ps1` | pass | builds/runs `MK_environment_commercial_readiness_v2_tests`; emits `environment_highest_commercial_ready=0`, `environment_commercial_ready=0`, `environment_commercial_ready_rows=8`, `environment_host_gated_rows=7`, `environment_unsupported_rows=1`, `environment_optimization_measurement_missing_artifacts=21`, and `environment_ready_unchanged=1` |
| 2026-06-19 | Task 12 final aggregate recipe dry-run | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode DryRun -Recipe environment-highest-commercial-readiness-closeout` | pass | routes through `tools/validate-environment-highest-commercial-readiness.ps1 -RequireReady` with 16 exact dependency rows, all six platform rows, 21 optimization measurement rows, visible-shell workflow evidence, `runtime_source_parsing=0`, and `environment_ready_unchanged=1` |
| 2026-06-19 | Task 12 validation recipe runner | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-validation-recipe-runner.ps1` | pass | `validation-recipe-runner-check: ok` |
| 2026-06-19 | Task 12 JSON contracts | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | pass | `agent-manifest-compose: ok`; `json-contract-check: ok` |
| 2026-06-19 | Task 13 CI matrix contract | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1` | pass | `ci-matrix-check: ok`; `validate.yml` now includes required `ios-metal` job in `pr-gate` |
| 2026-06-19 | Task 13 recipe/static contract | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-validation-recipe-runner.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-apple-host-evidence.ps1` | pass | recipe dry-run includes `ios_metal_command_buffer_ready=1`; JSON and AI surfaces are synced; Windows Apple evidence remains host-gated |
| 2026-06-19 | Task 13 highest commercial fail-closed validator | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-environment-highest-commercial-readiness.ps1` | pass | builds/runs `MK_environment_commercial_readiness_v2_tests`; emits `environment_highest_commercial_ready=0`, `environment_commercial_ready=0`, `environment_commercial_ready_rows=10`, `environment_host_gated_rows=5`, `environment_unsupported_rows=1`, `environment_metal_aggregate_ready=1`, `environment_platform_ios_metal_ready=1`, `environment_optimization_measurement_missing_artifacts=21`, and `environment_ready_unchanged=1` |
| 2026-06-19 | Task 13 full validation | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | pass | `validate: ok`; `100% tests passed, 0 tests failed out of 131`; Apple host checks remain host-gated/diagnostic-only on Windows |
| 2026-06-19 | Task 13 hosted iOS timeout hardening | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-apple-host-evidence.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-apple-metal-platform-host.ps1`; `git diff --check` | pass | CI now prints simulator devices, builds the iOS Simulator app in its own step, validates with `-SkipIosBuild`, preserves partial smoke output on timeout, and keeps Windows Apple evidence host-gated |
| 2026-06-19 | Task 13 hosted iOS timeout hardening contracts | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-validation-recipe-runner.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-environment-highest-commercial-readiness.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1` | pass | readiness remains fail-closed with `environment_commercial_ready_rows=10`, `environment_host_gated_rows=5`, `environment_unsupported_rows=1`, and `environment_ready=0`; format and text format checks pass |
| 2026-06-19 | Task 13 hosted iOS timeout hardening full validation | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | pass | `validate: ok`; `100% tests passed, 0 tests failed out of 131`; Apple host evidence remains host-gated on Windows and the hosted `ios-metal` PR check must provide hard Apple evidence |
| 2026-06-19 | Task 14 D3D12 typeperf artifact contract sync | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-validation-recipe-runner.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1` | pass | manifest/game/plan/static surfaces now record retained D3D12 typeperf artifacts, `environment_optimization_measurement_workload_rows=7`, `environment_optimization_measurement_backend_rows=1`, and `environment_optimization_measurement_missing_artifacts=14` |
| 2026-06-19 | Task 14 optimization artifact validator | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-environment-optimization-artifacts.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-environment-highest-commercial-readiness.ps1`; `git diff --check` | pass | retained D3D12 artifacts validate with `environment_optimization_measurement_profiler_artifacts=7`, `environment_optimization_measurement_trace_event_json=7`, `environment_optimization_measurement_missing_artifacts=14`, `environment_broad_optimization_ready=0`, and `environment_commercial_ready=0` |
| 2026-06-19 | Task 14 full validation | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | pass | `validate: ok`; `100% tests passed, 0 tests failed out of 131`; Windows Apple and Metal shader checks remain host-gated/diagnostic-only, and broad optimization remains fail-closed at `environment_optimization_measurement_missing_artifacts=14` |
| 2026-06-19 | Task 15 Vulkan strict VUID-clean package smoke | `pwsh -NoProfile -ExecutionPolicy Bypass -Command "& .\tools\package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game -RequireVulkanShaders -SmokeArgs @('--smoke','--max-frames','2','--require-config','runtime/sample_desktop_runtime_game.config','--require-scene-package','runtime/sample_desktop_runtime_game.geindex','--require-vulkan-renderer','--require-environment-vulkan-strict-aggregate','--require-debug-profiling-policy','--require-vulkan-debug-profiling-evidence','--require-environment-optimization-measurement')"` | pass | package smoke emits `environment_vulkan_strict_aggregate_ready=1`, `environment_optimization_measurement_backend=vulkan_strict`, and `environment_optimization_measurement_diagnostics=0` |
| 2026-06-19 | Task 15 Vulkan strict direct VUID scan | direct `sample_desktop_runtime_game.exe --smoke --max-frames 2 --require-config runtime/sample_desktop_runtime_game.config --require-scene-package runtime/sample_desktop_runtime_game.geindex --require-vulkan-renderer --require-environment-vulkan-strict-aggregate --require-debug-profiling-policy --require-vulkan-debug-profiling-evidence --require-environment-optimization-measurement` plus log scan for `Validation Error` / `VUID-` | pass | retained direct log has no `Validation Error` or `VUID-`; output includes `environment_vulkan_strict_aggregate_ready=1`, `environment_optimization_measurement_backend=vulkan_strict`, and `environment_broad_optimization_ready=0` |
| 2026-06-19 | Task 15 Vulkan strict focused tests | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_backend_scaffold_tests`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_backend_scaffold_tests` | pass | Vulkan runtime texture planning covers 3D image/view usage; `100% tests passed, 0 tests failed out of 1` |
| 2026-06-19 | Task 15 optimization artifact validator | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-environment-optimization-artifacts.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-environment-highest-commercial-readiness.ps1` | pass | retained D3D12 + strict Vulkan artifacts validate with `environment_optimization_measurement_workload_rows=14`, `environment_optimization_measurement_backend_rows=2`, `environment_optimization_measurement_profiler_artifacts=14`, `environment_optimization_measurement_missing_artifacts=7`, `environment_broad_optimization_ready=0`, `environment_commercial_ready_rows=10`, and `environment_commercial_ready=0` |
| 2026-06-19 | Task 15 contract and format sync | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-validation-recipe-runner.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1`; `git diff --check` | pass | manifest/game/plan/static surfaces now record retained D3D12 + strict Vulkan artifacts, `environment_optimization_measurement_workload_rows=14`, `environment_optimization_measurement_backend_rows=2`, and `environment_optimization_measurement_missing_artifacts=7` |
| 2026-06-19 | Task 15 full validation | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | pass | `validate: ok`; `100% tests passed, 0 tests failed out of 131`; Apple/Metal host checks remain host-gated or diagnostic-only on Windows, broad optimization remains fail-closed at `environment_optimization_measurement_missing_artifacts=7`, and commercial readiness remains `10/16` rows |
| 2026-06-19 | Task 16 Metal producer parse and Windows fail-closed check | `pwsh -NoProfile -Command '[scriptblock]::Create((Get-Content -Raw tools/generate-environment-metal-optimization-artifacts.ps1)) | Out-Null; "parse-ok"'`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/generate-environment-metal-optimization-artifacts.ps1` | pass | script parses; Windows emits `environment_metal_host_optimization_artifact_status=host_gated`, `xcrun_xctrace_ready=0`, `environment_metal_host_optimization_artifacts_written=0`, `environment_optimization_measurement_missing_artifacts=7`, `environment_broad_optimization_ready=0`, `environment_ready=0`, and `environment_commercial_ready=0` |
| 2026-06-19 | Task 16 Metal producer recipe dry-run | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode DryRun -Recipe environment-metal-host-optimization-artifact-producer` | pass | argv routes through `tools/generate-environment-metal-optimization-artifacts.ps1 -RequireReady` with `host=macos`, `host_gate=metal-apple`, `xcrun_xctrace_ready=1`, `xctrace_template=Metal_System_Trace`, seven artifact counters, `environment_broad_optimization_ready=1`, and non-promotion counters for `environment_ready` / `environment_commercial_ready` |
| 2026-06-19 | Task 16 static contracts and format | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-validation-recipe-runner.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1` | pass | producer recipe, macOS CI upload lane, manifest recipe, commercial-readiness static guard, agent integration, and text/C++ formatting are synchronized |
| 2026-06-19 | Task 16 readiness counters | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-environment-optimization-artifacts.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-environment-highest-commercial-readiness.ps1`; `git diff --check` | pass | retained D3D12 + strict Vulkan artifacts remain valid after LF normalization with `environment_optimization_measurement_workload_rows=14`, `environment_optimization_measurement_backend_rows=2`, `environment_optimization_measurement_invalid_hashes=0`, `environment_optimization_measurement_missing_artifacts=7`, `environment_broad_optimization_ready=0`; commercial aggregate remains `environment_commercial_ready_rows=10`, `environment_host_gated_rows=5`, `environment_unsupported_rows=1`, and `environment_commercial_ready=0` |
| 2026-06-19 | Task 16 full validation | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | pass | `validate: ok`; `100% tests passed, 0 tests failed out of 131`; Apple/Metal host checks remain host-gated or diagnostic-only on Windows, broad optimization remains fail-closed at 14/21 workload/backend rows, and commercial readiness remains `10/16` rows |
| 2026-06-20 | Task 17 retained Metal optimization artifacts | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-environment-optimization-artifacts.ps1` | pass | retained D3D12 + strict Vulkan + Apple-host Metal artifacts validate with `environment_optimization_measurement_workload_rows=21`, `environment_optimization_measurement_backend_rows=3`, `environment_optimization_measurement_profiler_artifacts=21`, `environment_optimization_measurement_trace_event_json=21`, `environment_optimization_measurement_missing_artifacts=0`, `environment_optimization_measurement_invalid_hashes=0`, `environment_optimization_measurement_over_budget=0`, and `environment_broad_optimization_ready=1` |
| 2026-06-20 | Task 17 commercial aggregate counters | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-environment-highest-commercial-readiness.ps1` | pass | broad optimization is ready, while commercial aggregate remains fail-closed with `environment_commercial_ready=0`; expected remaining blockers are Linux Vulkan, Android Vulkan, strict Vulkan aggregate, platform readiness, and all-platform readiness |
| 2026-06-20 | Task 18 Linux Vulkan hosted evidence contract | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-validation-recipe-runner.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | pass | `validate.yml` now includes required `linux-vulkan` PR gate membership, pinned checkout/upload actions, Ubuntu Vulkan diagnostic package installation, `vulkaninfo --summary`, `vulkaninfo --json`, `tools/validate-linux-vulkan-runtime-host.ps1`, and `linux-vulkan-host-evidence` artifact upload while manifest compose and recipe contracts remain synchronized |
| 2026-06-20 | Task 18 Windows fail-closed host check | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-linux-vulkan-runtime-host.ps1` | pass | Windows host emits `environment_platform_linux_vulkan_ready=0`, `environment_platform_requires_linux_vulkan_host_evidence=1`, `environment_all_platform_unconditional_ready=0`, `environment_platform_windows_vulkan_inferred=0`, and `native_handle_access=0` |
| 2026-06-20 | Task 18 agent-surface/static gate | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1`; `git diff --check`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 -StaticOnly -StaticJobs 1 -StaticCheckTimeoutSeconds 120` | pass | toolchain, agent config, AI integration, text format, whitespace, and full static validation pass; Windows Apple/Metal checks remain host-gated or diagnostic-only |
| 2026-06-20 | Task 19 Linux XCB Vulkan surface TDD RED | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_backend_scaffold_tests` | expected fail | missing `SurfacePlatform`, `SurfaceHandle::context`, and Linux XCB surface planning fields before implementation |
| 2026-06-20 | Task 19 Linux XCB Vulkan surface focused build | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_backend_scaffold_tests` | pass | built `MK_backend_scaffold_tests` after adding `SurfacePlatform::xcb`, `SurfaceHandle::context`, `VK_KHR_xcb_surface` planning, and private XCB probe plumbing |
| 2026-06-20 | Task 19 Linux XCB Vulkan surface focused CTest | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_backend_scaffold_tests` | pass | `100% tests passed, 0 tests failed out of 1`; tests cover Linux extension planning and missing XCB context/window fail-closed diagnostics |
| 2026-06-20 | Task 19 readiness counters | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-environment-highest-commercial-readiness.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-environment-optimization-artifacts.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-linux-vulkan-runtime-host.ps1` | pass | commercial aggregate remains fail-closed with `environment_commercial_ready_rows=11`, `environment_host_gated_rows=5`, `environment_platform_linux_vulkan_ready=0`, `environment_all_platform_unconditional_ready=0`, and `environment_broad_optimization_ready=1` |
| 2026-06-20 | Task 19 full validation | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | pass | `validate: ok`; `100% tests passed, 0 tests failed out of 131`; Apple/Metal host checks remain host-gated or diagnostic-only on Windows |
| 2026-06-20 | Task 20 Linux runtime host TDD RED | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_host_tests` | expected fail | missing `mirakana/runtime_host/linux/linux_desktop_game_host.hpp` before adding the Linux desktop host contract |
| 2026-06-20 | Task 20 Linux runtime host focused build | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_host_tests` | pass | built `MK_runtime_host_tests` after adding the Linux host value contract and `MK_runtime_host_linux` source tree |
| 2026-06-20 | Task 20 Linux runtime host focused CTest | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_host_tests` | pass | `100% tests passed, 0 tests failed out of 1`; tests cover invalid Linux host requests, non-Linux host gating, fallback reporting, and zero native-handle access |
| 2026-06-20 | Task 20 Linux host validator | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-linux-vulkan-runtime-host.ps1` | pass | Windows report remains host-gated with `environment_platform_linux_vulkan_ready=0` and `first_party_linux_runtime_host_ready=0`; the script now validates `engine/runtime_host/linux`, the public Linux runtime host header, and `MK_runtime_host_linux` CMake presence for the Linux-only first-party host row |
| 2026-06-20 | Task 23 Linux preset RED | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | expected fail | `CMakePresets.json` was missing `desktop-runtime-linux-release` before adding the Linux-only preset and package-script routing |
| 2026-06-20 | Task 23 static and agent-surface sync | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-validation-recipe-runner.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | pass | manifest compose, JSON contracts, recipe runner, format/text-format, agent config, and AI integration all pass after adding host-aware vcpkg bootstrap plus `desktop-runtime-linux-release` |
| 2026-06-20 | Task 23 dependency-policy RED/GREEN | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1` | pass | initial full validation exposed the old guard requiring `x64-windows` for every vcpkg preset; the guard now accepts `x64-linux` only for `desktop-runtime-linux-release` while keeping other vcpkg presets on `x64-windows` |
| 2026-06-20 | Task 23 Linux package fail-closed check | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-linux-runtime.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-linux-vulkan-runtime-host.ps1` | expected fail / pass | Windows package invocation fails closed with `Linux runtime packaging requires a Linux host`; the host validator emits `environment_platform_linux_vulkan_ready=0`, `environment_platform_requires_linux_vulkan_host_evidence=1`, `linux_package_smoke_ready=0`, `linux_vulkan_readback_ready=0`, and `environment_platform_windows_vulkan_inferred=0` |
| 2026-06-20 | Task 23 full validation | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | pass | `validate: ok`; `100% tests passed, 0 tests failed out of 131`; Apple/Metal checks remain host-gated or diagnostic-only on Windows, and Linux Vulkan remains host-gated |
| 2026-06-18 | Task 5 Metal platform TDD RED | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_env_platform_v2_tests` | expected fail | missing Metal/iOS fields in `EnvironmentPlatformEvidenceV2Row` |
| 2026-06-18 | Task 5 Metal platform focused build | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_env_platform_v2_tests` | pass | built `MK_env_platform_v2_tests` |
| 2026-06-18 | Task 5 Metal platform focused test | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_rhi_environment_platform_evidence_v2_tests` | pass | `100% tests passed, 0 tests failed out of 1` |
| 2026-06-18 | Task 5 Apple host validator Windows fail-closed | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-apple-metal-platform-host.ps1` | pass | `environment_platform_ios_metal_ready=0`, `environment_platform_requires_ios_metal_host_evidence=1`, `environment_all_platform_unconditional_ready=0` |
| 2026-06-18 | Task 5 iOS Metal recipe dry-run | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode DryRun -Recipe environment-platform-ios-metal-package` | pass | argv routes through `tools/validate-apple-metal-platform-host.ps1 -RequireReady -Platform ios` with exact iOS Metal expected counters |
| 2026-06-18 | Task 5 validation recipe runner | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-validation-recipe-runner.ps1` | pass | `validation-recipe-runner-check: ok` |
| 2026-06-18 | Task 5 Apple host static evidence | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-apple-host-evidence.ps1` | pass | reports Windows host-gated blockers and static iOS Metal evidence file/package hooks |
| 2026-06-18 | Task 6 backend parity v2 TDD RED | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_env_backend_parity_v2_tests` | expected fail | `explicit_missing_row_counts_as_missing_and_keeps_parity_0`: `result.missing_rows == 1U` |
| 2026-06-18 | Task 6 backend parity v2 focused validator | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-environment-backend-parity-v2.ps1 -RequireReady` | pass | `environment_backend_parity_v2_status=ready`, `environment_backend_parity_ready=1`, `environment_backend_parity_rows=45`, `environment_backend_parity_ready_rows=45`, `environment_backend_parity_all_platform_ready=0`, `environment_backend_parity_commercial_ready=0` |
| 2026-06-18 | Task 6 backend parity v2 recipe dry-run | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode DryRun -Recipe environment-backend-parity-v2-closeout` | pass | argv routes through `tools/validate-environment-backend-parity-v2.ps1 -RequireReady`; no host gate |
| 2026-06-18 | Task 6 validation recipe runner | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-validation-recipe-runner.ps1` | pass | `validation-recipe-runner-check: ok` |
| 2026-06-18 | Task 6 JSON contracts | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | pass | `agent-manifest-compose: ok`; `json-contract-check: ok` |
| 2026-06-18 | Task 6 agent integration | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | pass | `ai-integration-check: ok` |
| 2026-06-18 | Task 6 formatting | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | pass | `format-check: ok` |
| 2026-06-18 | Task 6 full validation | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | pass | `validate: ok`; `100% tests passed, 0 tests failed out of 130`; Apple host checks remain host-gated or diagnostic-only on Windows |
| 2026-06-18 | Task 8 preset asset library TDD RED | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_environment_tests` | expected fail | missing `EnvironmentPresetAssetLibraryProductionDesc` / `evaluate_environment_preset_asset_library_production` |
| 2026-06-18 | Task 8 preset asset library focused build | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_environment_tests` | pass | built `MK_environment_tests` |
| 2026-06-18 | Task 8 preset asset library focused test | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_environment_tests` | pass | `100% tests passed, 0 tests failed out of 1` |
| 2026-06-18 | Task 8 validation recipe runner | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-validation-recipe-runner.ps1` | pass | `validation-recipe-runner-check: ok` |
| 2026-06-18 | Task 8 recipe dry-run | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode DryRun -Recipe environment-aaa-preset-asset-library-production` | pass | argv routes through `tools/validate-environment-aaa-preset-asset-library.ps1 -RequireReady`; no host gate |
| 2026-06-18 | Task 8 dependency policy | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1` | pass | `dependency-policy-check: ok` |
| 2026-06-18 | Task 8 recipe execute | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe environment-aaa-preset-asset-library-production` | pass | builds/runs `MK_environment_tests`, emits `environment_aaa_preset_asset_library_ready=1`, `environment_aaa_preset_asset_library_asset_rows=156`, `environment_aaa_preset_asset_library_preview_screenshot_rows=144`, `environment_preset_asset_license_missing_rows=0`, `environment_preset_asset_package_budget_overages=0`, `environment_ready=0`, and `environment_commercial_ready=0` |
| 2026-06-18 | Task 8 JSON contracts | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | pass | `agent-manifest-compose: ok`; `json-contract-check: ok` |
| 2026-06-18 | Task 8 focused tidy | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/environment/src/environment_preset_pack.cpp,tests/unit/environment_tests.cpp` | pass | `tidy-check: ok (2 files)` |
| 2026-06-18 | Task 8 formatting | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | pass | `text-format-check: ok`; `format-check: ok` |
| 2026-06-18 | Task 8 public API boundaries | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | pass | `public-api-boundary-check: ok` |
| 2026-06-18 | Task 8 agent integration | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | pass | `ai-integration-check: ok` |
| 2026-06-18 | Task 8 agent config | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | pass | `agent-config-check: ok` |
| 2026-06-18 | Task 8 full validation | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | pass | `validate: ok`; `100% tests passed, 0 tests failed out of 130`; aggregate readiness remains blocked until all rows are ready |
| 2026-06-19 | Task 10 physical weather closeout TDD RED | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_environment_weather_simulation_tests` | expected fail | missing `EnvironmentPhysicalWeather*` closeout rows and `evaluate_environment_physical_weather_simulation_closeout` |
| 2026-06-19 | Task 10 focused build | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_environment_weather_simulation_tests` | pass | built `MK_environment_weather_simulation_tests` |
| 2026-06-19 | Task 10 focused CTest | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_environment_weather_simulation_tests` | pass | `100% tests passed, 0 tests failed out of 1` |
| 2026-06-19 | Task 10 weather physics validator | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-environment-weather-physics.ps1 -RequireReady` | pass | built and ran `MK_environment_weather_simulation_tests`, `MK_d3d12_environment_weather_solver_tests`, `MK_vulkan_environment_weather_solver_tests`, and `MK_metal_environment_weather_solver_tests`; `100% tests passed, 0 tests failed out of 4`; emitted `environment_physical_weather_simulation_ready=1`, `environment_weather_simulation_backend_parity_ready=1`, `environment_weather_simulation_validation_failures=0`, `environment_ready=0`, and `environment_commercial_ready=0` |
| 2026-06-19 | Task 10 recipe execute | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe environment-physical-weather-simulation-closeout` | pass | status `passed`; argv routes through `tools/validate-environment-weather-physics.ps1 -RequireReady`; emitted the same Task 10 ready and non-promotion counters |
| 2026-06-19 | Task 11 production artist workflow TDD RED | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_environment_tests` | expected fail | missing `environment_artist_workflow_v2.hpp` |
| 2026-06-19 | Task 11 focused build/test | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_environment_tests`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_editor_environment_tests` | pass | built and ran `MK_editor_environment_tests`; production closeout tests cover exact visible package rows and unsafe/weak evidence failure |
| 2026-06-19 | Task 11 validator | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-environment-artist-workflow-production.ps1 -RequireReady` | pass | builds/runs `MK_editor_environment_tests`, packages `sample_desktop_runtime_game`, and emits `environment_artist_workflow_production_ready=1`, `workflow_import_openexr_ready=1`, `workflow_live_preview_vulkan_ready=1`, `environment_artist_workflow_production_requirement_rows=14`, `environment_artist_workflow_production_ready_rows=14`, `environment_ready=0`, and `environment_commercial_ready=0` |
