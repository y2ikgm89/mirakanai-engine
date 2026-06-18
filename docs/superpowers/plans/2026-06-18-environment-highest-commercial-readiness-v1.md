# 2026-06-18 Environment Highest Commercial Readiness v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` or `superpowers:executing-plans` after the operator explicitly selects this plan for implementation. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `environment-highest-commercial-readiness-v1`

**Status:** Active. Task 1 selected this highest-level plan as `currentActivePlan`, created the exact spec, and records the Context7 gate as blocked with implementation code edits forbidden until every required Context7 row is ready. Task 1 landed in PR #669 with hosted validation passing. The 2026-06-18 Context7 gate continuation verified Vulkan and D3D12 rows, recorded the Metal row as partial because Context7 did not expose Apple framework API docs, and keeps implementation code edits forbidden. This file does not promote any readiness claim by itself.

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

Context7 is mandatory for SDK/API/dependency implementation. Task 1 originally recorded the exact refresh gate and official fallback sources while Context7 MCP tools were not exposed by tool discovery. The 2026-06-18 continuation confirmed the Context7 MCP tools are callable, but only the Vulkan and D3D12 rows are fully verified so far; implementation tasks remain blocked until every required row below is ready. If Context7 remains unavailable or a required Context7 row has no authoritative match when a code task starts, that task is blocked and must not edit implementation files.

Task 1 may still edit only plan, spec, docs, manifest fragments, composed manifest output, and the static checks that validate this active-plan selection to record the blocked Context7 status. Task 1 must not edit C++, shader, CMake, package, editor, renderer, importer, tool, or game runtime implementation files while `environment_highest_readiness_code_edit_allowed=0`.

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
| `context7.metal` | partial | `/dogukanveziroglu/metal-shading-language-specification` | 2026-06-18 | Context7 covers Metal shading language resource bindings and argument-buffer structures, but Apple framework API behavior for command queues, resources, feature tables, synchronization, and readback still requires official Apple documentation before Metal implementation work. |
| `context7.d3d12` | verified | `/websites/learn_microsoft_en-us_windows_win32_direct3d12` | 2026-06-18 | D3D12 rows must prove debug-layer or GPU-based-validation coverage where host-supported, explicit resource barriers, upload/readback heap evidence, texture state transitions, and no cross-backend inference. |

Current gate rows:

```text
environment_highest_readiness_context7_status=blocked
environment_highest_readiness_context7_missing_tools=0
environment_highest_readiness_context7_verified_rows=2
environment_highest_readiness_context7_partial_rows=1
environment_highest_readiness_context7_pending_rows=8
environment_highest_readiness_code_edit_allowed=0
```

Official fallback links checked for this plan on 2026-06-18:

- Vulkan validation overview: <https://docs.vulkan.org/guide/latest/validation_overview.html>
- Vulkan Profiles: <https://docs.vulkan.org/guide/latest/vulkan_profiles.html>
- Khronos Vulkan Validation Layers: <https://github.com/KhronosGroup/Vulkan-ValidationLayers>
- LunarG Vulkan SDK Linux getting started: <https://vulkan.lunarg.com/doc/sdk/latest/linux/getting_started.html>
- Android Vulkan validation layers: <https://developer.android.com/ndk/guides/graphics/validation-layer>
- Apple Metal Feature Set Tables: <https://developer.apple.com/metal/Metal-Feature-Set-Tables.pdf>
- Apple Metal argument buffers: <https://developer.apple.com/documentation/Metal/managing-groups-of-resources-with-argument-buffers>
- Apple Metal CPU performance with argument buffers: <https://developer.apple.com/documentation/metal/improving-cpu-performance-by-using-argument-buffers>
- Microsoft D3D12 resource barriers: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12>
- Microsoft D3D12 debug layer and GPU-based validation: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-d3d12-debug-layer-gpu-based-validation>
- Khronos glTF 2.0 specification: <https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html>
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

Current repository truth before selecting this plan:

- `docs/superpowers/plans/2026-06-13-environment-commercial-excellence-v1.md` is the active milestone.
- Selected D3D12 environment aggregate evidence exists.
- Selected strict Vulkan aggregate evidence exists for the reviewed Windows Vulkan package lane.
- Selected Apple-host Metal aggregate and macOS Metal platform evidence exist only behind Apple-host recipes.
- Selected package-visible backend parity is promoted only through the existing backend parity closeout.
- Linux Vulkan, iOS Metal, Android Vulkan, all-platform readiness, broad optimization, complete physical weather simulation, production solver readiness, external marketplace preset coverage, complete production artist workflow, commercial aggregate readiness, and broad `environment_ready` remain unclaimed.
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

Run the Context7 calls listed in `Official Source And Context7 Gate`. If any Context7 call is unavailable, keep this task limited to docs/spec/manifest selection and record:

```text
environment_highest_readiness_context7_status=blocked
environment_highest_readiness_context7_missing_tools=1
environment_highest_readiness_code_edit_allowed=0
```

No later task may edit implementation files until the same row is updated to:

```text
environment_highest_readiness_context7_status=ready
environment_highest_readiness_context7_missing_tools=0
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

- [ ] **Step 1: Write tests first**

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

- [ ] **Step 2: Implement clean-break API names**

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

- [ ] **Step 3: Validate focused tests**

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

- [ ] **Step 1: Add recipe ids**

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

- [ ] **Step 2: Add dry-run assertions**

Each recipe must have a dry-run row with command, host gate, and ready claim set to `0`. `environment-highest-commercial-readiness-closeout` must report:

```text
environment_highest_commercial_ready=0
environment_commercial_ready=0
environment_ready_promotion_blocked_until_all_rows_ready=1
```

- [ ] **Step 3: Validate recipe runner**

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

- [ ] **Step 1: Implement Linux Vulkan host gate**

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
environment_platform_linux_vulkan_ready=1
environment_platform_requires_linux_vulkan_host_evidence=0
```

- [ ] **Step 2: Implement Android Vulkan host gate**

The Android gate must require:

```text
host_has_android_sdk=1
host_has_android_ndk=1
adb_device_or_emulator_ready=1
android_vulkan_profile_ready=1
android_validation_layer_packaged=1
VK_LAYER_KHRONOS_validation_ready=1
android_package_smoke_ready=1
android_vulkan_readback_ready=1
environment_platform_android_vulkan_ready=1
```

- [ ] **Step 3: Keep Windows Vulkan separate**

Windows Vulkan remains its own strict package row. It must not set Linux or Android rows.

- [ ] **Step 4: Validate dry-run and no-inference guards**

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

## Task 5: Close Apple Metal Platform Rows

**Files:**
- Create: `tools/validate-apple-metal-platform-host.ps1`
- Modify: `engine/rhi/metal/**`
- Modify: `engine/runtime_rhi/include/mirakana/runtime_rhi/environment_platform_evidence_v2.hpp`
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `tools/run-validation-recipe-plans.ps1`

- [ ] **Step 1: Implement macOS Metal evidence**

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

- [ ] **Step 2: Implement iOS Metal evidence**

The iOS row must require:

```text
host=macos
xcode_ios_sdk_ready=1
ios_simulator_or_device_ready=1
ios_metal_feature_set_checked=1
ios_package_smoke_ready=1
ios_metal_command_queue_ready=1
ios_metal_pipeline_ready=1
ios_metal_readback_ready=1
environment_platform_ios_metal_ready=1
```

- [ ] **Step 3: Validate host-gated behavior on non-Apple hosts**

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

## Task 6: Promote Backend Parity v2 Only From Backend-Local Rows

**Files:**
- Create: `engine/renderer/include/mirakana/renderer/environment_backend_parity_v2.hpp`
- Create: `engine/renderer/src/environment_backend_parity_v2.cpp`
- Create: `engine/renderer/tests/environment_backend_parity_v2_tests.cpp`
- Modify: `tools/check-json-contracts-035-environment-commercial-readiness.ps1`

- [ ] **Step 1: Define exact parity matrix**

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

- [ ] **Step 2: Write fail-closed tests**

Tests must prove:

```text
d3d12_vulkan_ready_metal_missing_keeps_parity_0
macos_metal_ready_ios_metal_missing_keeps_all_platform_0
ready_rows_with_native_handle_access_keep_parity_0
ready_rows_with_diagnostics_keep_parity_0
all_backend_rows_ready_promotes_backend_parity_1
```

- [ ] **Step 3: Validate**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_renderer_tests
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

- [ ] **Step 1: Lock workloads**

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

- [ ] **Step 2: Require retained artifacts**

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

- [ ] **Step 3: Require budgets**

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

- [ ] **Step 4: Validate artifact checker**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-environment-optimization-artifacts.ps1 -RequireReady
```

Expected when artifacts are incomplete:

```text
environment_broad_optimization_ready=0
environment_optimization_measurement_missing_artifacts>0
```

Expected at closeout:

```text
environment_broad_optimization_ready=1
environment_optimization_measurement_missing_artifacts=0
```

## Task 8: Build The AAA Preset Asset Library As Objective Rows

**Files:**
- Modify: `engine/environment/**`
- Modify: `engine/tools/**`
- Modify: `editor/core/**`
- Modify: `games/sample_desktop_runtime_game/game.agent.json`
- Modify: `docs/legal-and-licensing.md`
- Modify: `THIRD_PARTY_NOTICES.md`

- [ ] **Step 1: Define non-subjective asset counts**

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

- [ ] **Step 2: Prefer first-party assets**

Use first-party generated or hand-authored content for default preset packs. Any third-party asset requires an entry in `THIRD_PARTY_NOTICES.md` with name, source URL, retrieved date, version or commit, copyright holder, SPDX license, modification status, and distribution target.

- [ ] **Step 3: Validate**

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

## Task 9: Promote Full OpenEXR/KTX2/Basis Asset Pipeline

**Files:**
- Create: `engine/tools/include/mirakana/tools/environment_texture_pipeline_v2.hpp`
- Create: `engine/tools/asset/environment_texture_pipeline_v2.cpp`
- Modify: `vcpkg.json`
- Modify: `docs/dependencies.md`
- Modify: `docs/legal-and-licensing.md`
- Modify: `THIRD_PARTY_NOTICES.md`
- Modify: `tools/bootstrap-deps.ps1`
- Modify: `tools/check-dependency-policy.ps1`

- [ ] **Step 1: Define supported source matrix**

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

- [ ] **Step 2: Keep dependencies optional**

`vcpkg.json` must keep OpenEXR and KTX behind a manifest feature named `asset-importers`. `tools/bootstrap-deps.ps1` remains the only dependency installation entrypoint. CMake configure must not download, restore, or install packages.

- [ ] **Step 3: Validate optional feature**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1 -Feature asset-importers
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-asset-importers.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe environment-asset-pipeline-openexr-ktx-basis-full
```

Expected:

```text
environment_asset_pipeline_openexr_ktx_basis_full_ready=1
environment_asset_pipeline_runtime_source_parsing=0
environment_asset_pipeline_dependency_gated_rows=0
```

## Task 10: Close Physical Weather Simulation

**Files:**
- Modify: `engine/environment/**`
- Modify: `engine/renderer/**`
- Modify: `engine/rhi/d3d12/**`
- Modify: `engine/rhi/vulkan/**`
- Modify: `engine/rhi/metal/**`
- Create: `tools/validate-environment-weather-physics.ps1`

- [ ] **Step 1: Define simulation scope**

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

- [ ] **Step 2: Require solver evidence**

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

- [ ] **Step 3: Validate dataset provenance**

Weather dataset import/review rows must use CF/netCDF, GRIB, or synthetic first-party validation fixtures with recorded provenance. External dataset files require license and redistribution records before becoming package artifacts.

- [ ] **Step 4: Validate**

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

## Task 11: Close Production Artist Workflow

**Files:**
- Create: `editor/core/include/mirakana/editor/environment_artist_workflow_v2.hpp`
- Create: `editor/core/src/environment_artist_workflow_v2.cpp`
- Create: `editor/core/tests/environment_artist_workflow_v2_tests.cpp`
- Create: `editor/src/environment_artist_workflow_shell_v2.cpp`
- Modify: `editor/src/**`
- Modify: `games/sample_desktop_runtime_game/game.agent.json`

- [ ] **Step 1: Define visible workflow rows**

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

- [ ] **Step 2: Keep editor-core side effects blocked**

`MK_editor_core` may build value models, command plans, and reports. It must not execute package scripts, validation recipes, GPU commands, filesystem mutation outside reviewed document apply paths, native handles, or dependency tools.

- [ ] **Step 3: Validate shell execution**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_editor_core_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe environment-artist-workflow-production-closeout
```

Expected:

```text
environment_artist_workflow_production_ready=1
environment_artist_workflow_editor_core_backend_execution=0
environment_artist_workflow_native_handle_access=0
```

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

- [ ] **Step 1: Require all dependency rows**

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

- [ ] **Step 2: Add static guard against broad false positives**

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

Do not merge Task 12 until every prior PR has hosted evidence or an explicit host-gated blocker row has been removed by evidence.

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
