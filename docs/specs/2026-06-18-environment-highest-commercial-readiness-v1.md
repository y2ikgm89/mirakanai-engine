# Environment Highest Commercial Readiness v1 Specification

## Status

Active specification for `environment-highest-commercial-readiness-v1`. This specification selects the clean-break highest-level environment commercial readiness milestone, records the official-source lock, and defines the exact readiness rows that later implementation tasks must prove. It is not a readiness completion claim.

## Goal

Promote the environment capability to commercial readiness only when strict Vulkan, Apple Metal, backend parity, exact all-platform readiness, measured optimization, AAA preset assets, full OpenEXR/KTX2/Basis production asset ingestion, physically based weather simulation, and production artist workflow evidence are all package-visible and fail-closed.

## Current Task 1 Gate

Context7 is required before SDK/API/dependency implementation work. The callable Context7 MCP tools are not exposed in this session after tool discovery and install-candidate discovery.

Task 1 therefore records:

```text
environment_highest_readiness_context7_status=blocked
environment_highest_readiness_context7_missing_tools=1
environment_highest_readiness_code_edit_allowed=0
```

Task 1 may edit only:

```text
docs/specs/2026-06-18-environment-highest-commercial-readiness-v1.md
docs/superpowers/plans/2026-06-18-environment-highest-commercial-readiness-v1.md
docs/superpowers/plans/2026-06-13-environment-commercial-excellence-v1.md
docs/superpowers/plans/README.md
docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md
docs/roadmap.md
engine/agent/manifest.fragments/010-aiOperableProductionLoop.json
engine/agent/manifest.json
tools/check-ai-integration-010-agent-baseline.ps1
tools/check-ai-integration-020-engine-manifest.ps1
tools/check-ai-integration-030-runtime-rendering.ps1
tools/check-ai-integration-094-runtime-sandbox-world-pack.ps1
tools/check-ai-integration-095-runtime-simulation-management-pack.ps1
tools/check-ai-integration-096-runtime-network-replication-pack.ps1
tools/check-ai-integration-097-rendering-vfx-profiling-pack.ps1
tools/check-ai-integration-104-mavg-runtime-lod.ps1
tools/check-ai-integration-105-mavg-gpu-culling-indirect.ps1
tools/check-ai-integration-106-mavg-rhi-indirect-draw.ps1
tools/check-ai-integration-107-mavg-d3d12-indexed-indirect-draw-execution.ps1
tools/check-ai-integration-108-mavg-vulkan-indexed-indirect-draw-execution.ps1
tools/check-ai-integration-109-mavg-d3d12-count-buffer-indirect-execution.ps1
tools/check-ai-integration-110-mavg-vulkan-count-buffer-indirect-execution.ps1
tools/check-ai-integration-113-mavg-vulkan-gpu-culling-dispatch.ps1
tools/check-ai-integration-114-mavg-vulkan-compute-generated-indirect-consumption.ps1
tools/check-ai-integration-115-mavg-payload-byte-range-page-loader.ps1
tools/check-ai-integration-120-mavg-resident-page-recency-eviction-order.ps1
tools/check-ai-integration-121-mavg-streaming-upload-overlap-evidence.ps1
tools/check-json-contracts-030-tooling-contracts.ps1
tools/check-json-contracts-040-agent-surfaces.ps1
```

No C++, shader, CMake, package, editor, renderer, importer, tool, or game runtime implementation file may be edited while `environment_highest_readiness_code_edit_allowed=0`.

## Final Readiness Counter Contract

The final commercial closeout may emit the rows below only in the same package-visible commercial closeout report:

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

Required dependency rows:

| Row | Required value | Proof boundary |
| --- | --- | --- |
| `environment_strict_vulkan_aggregate_ready` | `1` | Windows, Linux, and Android Vulkan rows each have backend-local execution, validation-layer, SPIR-V validation, synchronization2, dynamic-rendering, readback, and zero-diagnostic evidence. |
| `environment_metal_aggregate_ready` | `1` | macOS and iOS Metal rows each have Apple-host command queue, command buffer, pipeline, resource usage, synchronization/readback, feature-table/capability, and zero-diagnostic evidence. |
| `environment_backend_parity_ready` | `1` | D3D12, Vulkan, and Metal rows all satisfy the same environment feature matrix without transferring proof across APIs or hosts. |
| `environment_platform_readiness_ready` | `1` | Exact platform rows for Windows D3D12, Windows Vulkan, Linux Vulkan, macOS Metal, iOS Metal, and Android Vulkan are ready. |
| `environment_broad_optimization_ready` | `1` | Seven named workloads are measured before/after on D3D12, strict Vulkan, and Apple-host Metal, with retained profiler artifacts and no over-budget rows. |
| `environment_aaa_preset_asset_library_ready` | `1` | First-party or explicitly licensed production preset packs meet count, provenance, package, screenshot, budget, and artist workflow criteria. |
| `environment_asset_pipeline_openexr_ktx_basis_full_ready` | `1` | Source-to-cooked pipeline handles the declared OpenEXR and KTX2/Basis matrix, device format selection, color metadata, package replay, and runtime cooked-only ingest. |
| `environment_physical_weather_simulation_ready` | `1` | Coupled CPU reference and GPU solver rows meet validation dataset, conservation, budget, visual regression, and backend parity thresholds. |
| `environment_artist_workflow_production_ready` | `1` | Visible editor shell supports import, cook, package, preview, compare, validate, report, revise, undo/redo, batch, and review flows without editor-core backend execution. |

## Clean-Break Row Policy

Commercial v2 rows are new rows.

Existing selected rows are input evidence only.

No rename, alias, compatibility bridge, or implicit carry-forward is allowed.

| Existing selected row | Commercial v2 row | Allowed use |
| --- | --- | --- |
| `environment_vulkan_strict_aggregate_ready` | `environment_strict_vulkan_aggregate_ready` | Historical input evidence only. The owning task must re-emit Windows, Linux, and Android Vulkan platform rows plus the v2 aggregate in its own package-visible report. |
| `environment_metal_host_aggregate_ready` | `environment_metal_aggregate_ready` | Historical Apple-host input evidence only. The owning task must re-emit macOS and iOS Metal platform rows plus the v2 aggregate in its own package-visible report. |
| `environment_aaa_preset_library_ready` | `environment_aaa_preset_asset_library_ready` | Selected package evidence only. The owning task must prove the larger production asset-library count, provenance, screenshots, budgets, and workflow rows. |
| `environment_asset_pipeline_openexr_ktx_basis_ready` | `environment_asset_pipeline_openexr_ktx_basis_full_ready` | Selected source-to-package evidence only. The owning task must prove the full declared image, compression, metadata, runtime cooked-only, replay, and backend target matrix. |
| `environment_artist_workflow_ready` | `environment_artist_workflow_production_ready` | Selected workflow evidence only. The owning task must prove visible shell execution, revision safety, batch, compare, undo/redo, and package validation rows. |

## Forbidden Promotion Paths

- D3D12 evidence must not promote Vulkan, Metal, all-platform, broad optimization, physical weather, or commercial readiness.
- Windows Vulkan evidence must not promote Linux Vulkan or Android Vulkan.
- macOS Metal evidence must not promote iOS Metal.
- Earlier selected rows may seed review inputs, but every commercial v2 row must be re-emitted by its owning task with fresh package-visible counters before it can participate in the final aggregate.
- Backend parity must not promote all-platform readiness unless all exact platform rows are ready.
- Asset-library row counts must not promote AAA readiness without provenance, package, validation, screenshot, and budget rows.
- Weather visual quality review must not promote physical simulation without solver validation, conservation, and backend parity rows.
- Artist workflow value models must not promote production workflow without visible shell execution, revision safety, batch evidence, and package validation.
- This plan does not promote broad `environment_ready=1`.

## Official Source Lock

Context7 calls remain required before later SDK/API/dependency implementation. Until the tools are exposed, implementation must use these official fallback sources only as planning anchors and must stay docs/manifest-only:

| Area | Source | Implementation implication |
| --- | --- | --- |
| Vulkan validation | <https://docs.vulkan.org/guide/latest/validation_overview.html> | Strict Vulkan rows must require `VK_LAYER_KHRONOS_validation` evidence and zero validation diagnostics. |
| Vulkan Profiles | <https://docs.vulkan.org/guide/latest/vulkan_profiles.html> | Vulkan readiness must pin feature/profile capability evidence instead of assuming device support. |
| Vulkan Validation Layers | <https://github.com/KhronosGroup/Vulkan-ValidationLayers> | Validation layer setup is part of the toolchain proof, not optional narrative evidence. |
| LunarG Vulkan SDK | <https://vulkan.lunarg.com/doc/sdk/latest/linux/getting_started.html> | Linux Vulkan rows must prove SDK/tool availability through official SDK workflow. |
| Android Vulkan validation | <https://developer.android.com/ndk/guides/graphics/validation-layer> | Android Vulkan rows must prove packaged/deployed validation-layer evidence on Android, not desktop inference. |
| Apple Metal feature sets | <https://developer.apple.com/metal/Metal-Feature-Set-Tables.pdf> | Metal rows must cite Apple host feature capability for macOS and iOS separately. |
| Apple Metal argument buffers | <https://developer.apple.com/documentation/Metal/managing-groups-of-resources-with-argument-buffers> | Resource-group decisions must be Apple-host validated and backend-private. |
| Apple Metal CPU argument-buffer performance | <https://developer.apple.com/documentation/metal/improving-cpu-performance-by-using-argument-buffers> | Artist/runtime performance rows must retain measurable Apple-host evidence before promotion. |
| D3D12 resource barriers | <https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12> | D3D12 rows must keep resource-state and barrier proof backend-local. |
| D3D12 debug layer and GPU-based validation | <https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-d3d12-debug-layer-gpu-based-validation> | D3D12 diagnostic proof must use official debug-layer/GPU-based-validation evidence. |
| glTF 2.0 | <https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html> | Runtime asset delivery must preserve glTF/KTX2 metadata boundaries when used. |
| KTX | <https://www.khronos.org/ktx/> | KTX2/Basis rows must prove container/transcode/format decisions explicitly. |
| OpenEXR | <https://openexr.com/en/latest/TechnicalIntroduction.html> | OpenEXR rows must preserve scene-linear HDR metadata and channel/pixel-type evidence. |
| OpenUSD | <https://openusd.org/dev/intro.html> | Production asset library/pipeline rows must treat USD as a structured interchange source, not ad hoc text parsing. |
| MaterialX | <https://materialx.org/> | Production look-development rows must keep material graph exchange explicit. |
| OpenColorIO | <https://opencolorio.org/> | Color management rows must preserve OCIO/ACES display and view-transform evidence. |
| CF conventions | <https://cfconventions.org/> | Weather dataset rows must define coordinate, unit, and metadata interpretation. |
| NOAA Weather and Climate Toolkit | <https://www.ncei.noaa.gov/products/weather-climate-toolkit> | Weather validation datasets must come from reviewable weather/climate data tooling assumptions. |
| ECMWF ecCodes | <https://www.ecmwf.int/en/learning/training/eccodes-grib-and-bufr-data-decoding-and-encoding-software> | GRIB/BUFR handling must be explicit optional tooling work before ingestion claims. |
| vcpkg manifest mode | <https://learn.microsoft.com/en-us/vcpkg/concepts/manifest-mode> | Optional dependencies stay in manifest features and are not restored during CMake configure. |
| `vcpkg.json` reference | <https://learn.microsoft.com/en-us/vcpkg/reference/vcpkg-json> | Dependency version/feature records must remain reproducible and pinned. |

## Fixed Optimization Evidence Contract

The workload matrix is:

```text
preset_pack_flythrough
storm_precipitation
dense_volumetric_fog
volumetric_cloud_sunset
snowfield_material_weathering
weather_simulation_stress
asset_library_cold_load
```

The backend matrix is:

```text
d3d12
vulkan_strict
metal_apple_host
```

Promotion requires 21 workload/backend rows, each with before/after CPU/GPU/memory/upload/stutter/pipeline-cache measurements, retained profiler and trace artifacts, a 64-character lowercase SHA-256 hash, and artifact paths under:

```text
artifacts/environment/optimization/<task-id>/<backend>/<workload>/
```

## Architecture Boundaries

| Module | Boundary |
| --- | --- |
| `MK_environment` | Backend-neutral environment and weather contracts, validation, and package-visible counters. No GPU execution. |
| `MK_tools` | Source import, cook, package mutation, optional dependency adapters, and source-to-cooked transforms. |
| `MK_renderer` | Renderer-neutral policy, parity matrices, and public renderer-facing validation rows. |
| `MK_rhi_*` | Backend-private D3D12, Vulkan, and Metal execution. Native handles remain private. |
| `MK_editor_core` | Reviewable workflow value models and retained row contracts. No backend execution. |
| `MK_editor` | Visible first-party shell execution and artist workflow orchestration. |

## Validation

Task 1 validation requires:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
```
