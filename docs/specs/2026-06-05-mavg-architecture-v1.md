# 2026-06-05 MAVG Architecture v1

## Purpose

Define the clean-room architecture baseline for Mirakana Adaptive Virtual Geometry (MAVG) and the first data-only implementation boundary. This document is a specification and audit record, not a renderer/runtime ready-claim. The first implementation child adds a cooked cluster graph descriptor and package planner only; it does not add a renderer path, backend feature, streaming system, deformation system, ray tracing payload, or benchmark result.

## Status

Phase 0 specification completed for `mavg-research-legal-benchmark-baseline-v1`. The stacked implementation milestone `mavg-runtime-lod-milestone-v1` is now completed over the `mavg-asset-graph-v1` foundation, and `mavg-page-streaming-queue-v1` plus `mavg-page-streaming-eviction-review-v1` add caller-reviewed page request queue and selected/fallback page eviction review evidence without reopening that completed milestone. The completed LoD path implements deterministic `MK_assets` `GameEngine.MavgClusterGraph.v1` hierarchy/error/fallback/draw-range graph validation, `MK_tools` static `GameEngine.MavgClusterPayload.v1` vertex/index payload rows through `MavgClusterCookVertex`, `vertex.data_hex`, `index.data_hex`, per-material root/leaf fallback clusters, `MK_renderer` CPU reference selection through `mavg_lod_selection.hpp` / `select_mavg_lod_clusters`, `MK_runtime` resident-page evidence through `mavg_lod_residency.hpp` / `build_runtime_mavg_lod_residency`, `MK_runtime` caller-reviewed page request planning, selected/fallback page eviction review, and one-row safe-point drain through `mavg_page_streaming.hpp` / `plan_runtime_mavg_page_streaming_requests` / `review_runtime_mavg_page_streaming_evictions` / `execute_runtime_mavg_page_streaming_request_safe_point`, range-aware conventional indexed draws through `MeshIndexedDrawRange` and the clean-break 5-argument `rhi::IRhiCommandList::draw_indexed`, and `MK_scene_renderer` conventional `MeshCommand` planning through `mavg_scene_lod.hpp` / `plan_mavg_scene_lod_mesh_commands`. Autonomous/background package streaming execution, automatic eviction policy, partial `.mavgpayload` byte-range page loading/schema, GPU memory pressure integration, GPU culling, indirect draw execution, mesh shaders, deformation, ray tracing, Metal readiness, benchmark superiority, and Nanite compatibility/equivalence/superiority remain unclaimed until later focused tasks add code and validation evidence.

## Current Repository Baseline

- `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` selects `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`; the stacked LoD milestone is completed and follow-up MAVG slices are recorded as separate implementation plans.
- `unsupportedProductionGaps` remains `[]`; MAVG is post-1.0 clean-break research/specification work.
- SDL3 is not an active dependency or supported runtime/editor/audio path.
- First-party Windows desktop foundations are `MK_platform_win32`, `MK_runtime_host_win32`, `MK_runtime_host_win32_presentation`, and `MK_audio_wasapi`.
- First-party UI/editor foundations are `MK_ui`, `MK_ui_renderer`, `MK_editor_core`, `mirakana::ui`, and the private Win32/Direct3D 12/DirectWrite/TSF/UIA editor shell adapters.
- `MK_environment` is a current renderer and scene-renderer dependency. MAVG benchmark scenes must include selected environment load where relevant, without claiming broad `environment_ready`.
- Performance foundation rows already exist for performance budgets, memory diagnostics, scratch arenas, job execution, CPU placement, SIMD/AVX2 dispatch, CPU profiling matrix, optional GPU compute review, and long-run readiness.

## Clean-Room Policy

Allowed inputs:

- Public official API documentation and specifications.
- Public academic papers about simplification, progressive meshes, meshlets, clustering, and GPU-driven rendering.
- Public vendor guidance used only as constraints or design background.
- First-party repository source code and generated first-party assets.

Forbidden inputs:

- Unreal Engine source code, Nanite source code, UE shaders, UE cooked data, private Epic presentations, or reverse-engineered UE implementation details.
- Third-party source snippets from blogs, Stack Overflow, books, samples, GitHub repositories, or PDFs unless the license is explicit and dependency/legal records are updated.
- Product/API names that imply Nanite compatibility or UE compatibility.
- Vendor-only APIs in the default path.

Commercial distribution gate:

- Before marketing claims that MAVG exceeds a named commercial technology, run a freedom-to-operate review with counsel or a patent attorney.

## Official Source Matrix

| Area | Required source checks before implementation | Architecture constraint |
| --- | --- | --- |
| Epic Nanite feature taxonomy | `https://dev.epicgames.com/documentation/en-us/unreal-engine/nanite-virtualized-geometry-in-unreal-engine` | Use as feature/support taxonomy only; do not copy implementation or naming. |
| D3D12 mesh shaders | `https://microsoft.github.io/DirectX-Specs/d3d/MeshShader.html` | Query support and limits; keep compute/indirect fallback. |
| D3D12 Work Graphs | `https://microsoft.github.io/DirectX-Specs/d3d/WorkGraphs.html` | Research-gated after baseline GPU paths. |
| D3D12 resource barriers | `https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12` and `https://microsoft.github.io/DirectX-Specs/d3d/D3D12EnhancedBarriers.html` | Record explicit compute-write to indirect/draw-read barriers. |
| Vulkan mesh shader | `https://docs.vulkan.org/features/latest/features/proposals/VK_EXT_mesh_shader.html` and `https://registry.khronos.org/vulkan/specs/latest/man/html/VK_EXT_mesh_shader.html` | Host/toolchain gate `VK_EXT_mesh_shader`; no proof inheritance from D3D12. |
| Vulkan synchronization | `https://docs.vulkan.org/guide/latest/synchronization.html` and Context7 `/khronosgroup/vulkan-docs` | Use synchronization2-era reasoning and strict validation evidence. |
| Metal mesh/object shaders | `https://developer.apple.com/metal/capabilities/` and `https://developer.apple.com/documentation/metal/mesh-and-object-shader-resource-preparation-commands` | Apple-host gated; no Windows inference. |
| CMake and C++ modules | `https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html`, `https://cmake.org/cmake/help/latest/manual/cmake-cxxmodules.7.html`, and Context7 `/kitware/cmake` | Preserve tracked `CMakePresets.json`, local `CMakeUserPresets.json`, install/export/file-set discipline. |
| vcpkg manifest mode | `https://learn.microsoft.com/en-us/vcpkg/concepts/manifest-mode`, `https://learn.microsoft.com/en-us/vcpkg/reference/vcpkg-json`, and Context7 `/microsoft/vcpkg` | Optional dependencies stay explicit feature gates with legal records. |
| Win32 input/audio/text/accessibility | Raw Input, WASAPI/Core Audio, DirectWrite, TSF, and UIA Microsoft Learn docs | MAVG tools use private first-party adapters; no public native handles. |
| Vendor meshlet guidance | NVIDIA RTX Mega Geometry public guidance and AMD GPUOpen meshlet compression guidance | Background only; default path remains first-party and cross-vendor. |

## Subsystem Boundaries

### Asset And Cook

The first implementation files should start in `MK_assets` and `MK_tools`:

- `engine/assets/include/mirakana/assets/mavg_cluster_graph.hpp`
- `engine/assets/src/mavg_cluster_graph.cpp`
- `engine/tools/include/mirakana/tools/mavg_cluster_cook.hpp`
- `engine/tools/asset/mavg_cluster_cook.cpp`

Implemented v1 responsibilities:

- Define deterministic static cluster graph rows through `MavgClusterGraphDocument`.
- Validate asset ids, source mesh refs, source/payload URIs, material partitions, page rows, cluster rows, bounds, child refs, parent/root hierarchy, geometric error monotonicity, resident fallback ancestry, draw ranges, duplicate ids, and package dependency edge kinds.
- Serialize and deserialize `GameEngine.MavgClusterGraph.v1` text with canonical page/material/cluster ordering.
- Produce first-party graph descriptor, deterministic static payload evidence, changed-file rows, and `.geindex` package metadata through `MavgClusterCookRequest` / `MavgClusterCookResult`.
- Emit `MavgClusterCookVertex` position/normal/UV rows and indexed `MavgClusterCookTriangle` rows into `GameEngine.MavgClusterPayload.v1` `vertex.data_hex` / `index.data_hex` little-endian payload rows.
- Build simple per-material root/leaf fallback clusters whose parent/root geometric error stays monotonic.
- Reject invalid inputs, unsafe package paths, missing dependency package rows, and malformed graph/package rows before emitting changed files.

Non-responsibilities:

- Renderer backend execution.
- Streaming IO execution.
- Runtime source import.
- Third-party simplifier ownership.
- Renderer submission and backend execution; CPU selection now belongs to the `MK_renderer` selector checkpoint and resident-page evidence now belongs to the `MK_runtime` bridge checkpoint in the active LoD milestone.

The detailed LoD milestone `docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md` is completed for graph, draw-ready static cook payload, CPU selector, runtime resident-page evidence, range-aware conventional indexed draws, and conventional scene submission planning. `docs/superpowers/plans/2026-06-06-mavg-page-streaming-queue-v1.md` records the first post-milestone package-streaming follow-up slice.

### Runtime Selection

The implemented CPU reference selector files live in `MK_renderer`:

- `engine/renderer/include/mirakana/renderer/mavg_lod_selection.hpp`
- `engine/renderer/src/mavg_lod_selection.cpp`

Implemented v1 responsibilities:

- CPU reference selection over finite view rows, screen-space error, resident fallback, budget degradation, and temporal hysteresis.
- Value-only selected cluster rows, missing page rows, fallback substitution rows, and quality diagnostics.
- Deterministic behavior under incomplete residency.

Non-responsibilities:

- GPU culling.
- Backend resource allocation.
- Package streaming.

### Runtime Resident Page Evidence

The implemented resident-page bridge files live in `MK_runtime` while shared page-set/request rows live in `MK_assets`:

- `engine/assets/include/mirakana/assets/mavg_cluster_graph.hpp`
- `engine/runtime/include/mirakana/runtime/mavg_lod_residency.hpp`
- `engine/runtime/src/mavg_lod_residency.cpp`

Implemented v1 responsibilities:

- Convert already-reviewed `RuntimeResourceCatalogV2` `AssetKind::mavg_cluster_graph` rows plus a caller-owned `MavgClusterGraphDocument` observer into `MavgLodResidentPageSet` rows.
- Validate selector `MavgLodPageRequest` rows against the graph asset and known graph page indices, then preserve accepted rows as reviewed page requests for a later streaming plan.
- Report missing catalog/graph/catalog-kind evidence without reading package files, mutating resident mount sets, executing streaming, or touching renderer/RHI handles.

Non-responsibilities:

- Loading package files or parsing runtime sources.
- Background streaming or automatic eviction.
- Renderer/RHI upload, residency, or native handle ownership.

### Runtime Page Streaming Queue

The implemented caller-reviewed page streaming queue files live in `MK_runtime` and use existing runtime package safe-point helpers:

- `engine/runtime/include/mirakana/runtime/mavg_page_streaming.hpp`
- `engine/runtime/src/mavg_page_streaming.cpp`

Implemented v1 responsibilities:

- Expose `RuntimeMavgPageStreamingCandidateRow`, `RuntimeMavgPageStreamingPlanResult`, and `RuntimeMavgPageStreamingDrainResult` as the public value/evidence rows for planning and safe-point drain results.
- Convert reviewed selector `MavgLodPageRequest` rows into deterministic `RuntimeMavgPageStreamingPlanRow` package candidate rows.
- Validate graph asset ids, graph/page membership, finite priorities, non-empty reasons, candidate graph matches, duplicate candidates, and missing candidates before any file IO.
- Coalesce duplicate page requests by highest priority, sort queued rows deterministically by priority then page index, and report `max_queued_pages` degradation.
- Drain one queued row at a caller-owned safe point through `commit_runtime_package_candidate_resident_mount_with_reviewed_evictions_v2`.
- Preserve live mount/catalog state when the delegated safe point rejects invalid mount ids, package load, eviction plan, budget, or catalog refresh.
- Report explicit side-effect flags proving planner file IO, planner mount mutation, planner streaming execution, background worker execution, and renderer/RHI handle access remain false.
- Expose `RuntimeMavgResidentPageMountRow`, `RuntimeMavgPageStreamingSelectedClusterRow`, and `RuntimeMavgPageStreamingEvictionReviewResult` for caller-reviewed eviction review before a page-streaming safe point.
- Protect selected visible cluster page mounts and resident fallback ancestor page mounts before delegating to `plan_runtime_resident_package_evictions_v2`.
- Reject graph mismatches, unknown selected clusters, unknown pages, invalid/duplicate/missing resident page mounts, invalid/duplicate caller-protected mounts, and protected eviction candidates without mutating resident state.

Non-responsibilities:

- autonomous/background package streaming workers.
- Automatic eviction policy.
- Partial `.mavgpayload` byte-range page loading/schema.
- GPU memory pressure integration.
- Renderer/RHI upload, GPU culling, indirect draws, mesh shaders, or native handle ownership.

### Conventional Renderer Adoption

The completed conventional scene-submission path avoids backend-specific GPU work and lives in `MK_scene_renderer` while using existing renderer/RHI opaque binding contracts:

- `engine/scene_renderer/include/mirakana/scene_renderer/mavg_scene_lod.hpp`
- `engine/scene_renderer/src/mavg_scene_lod.cpp`

Implemented responsibilities:

- Plan selected clusters into existing `MeshCommand` rows through `SceneGpuBindingPalette`.
- Submit only selected cluster index ranges through `MeshIndexedDrawRange`.
- Preserve existing `MeshGpuBinding` / `MaterialGpuBinding` ownership and opaque handle boundaries.
- Report missing material bindings and fallback substitutions as diagnostics without exposing native/RHI handles.

Future responsibilities:

- Automate selected cluster payload upload through existing upload/frame-graph paths if a later package-streaming or runtime-upload plan selects it.
- Report package-visible MAVG runtime counters without mesh shader dependency.

### GPU Culling And Indirect

Future files should start in `MK_rhi` and `MK_renderer`:

- `engine/rhi/include/mirakana/rhi/indirect_draw.hpp`
- `engine/renderer/include/mirakana/renderer/mavg_gpu_culling.hpp`
- backend-local implementation files under `engine/rhi/d3d12` and `engine/rhi/vulkan`.

Responsibilities:

- Backend-neutral indirect command rows.
- Compute culling buffer contracts.
- D3D12 and Vulkan synchronization evidence.
- Null RHI deterministic counters.

### Mesh Shader Backends

Future files should start in `shaders/`, `MK_renderer`, and backend-local `MK_rhi_*` code.

Responsibilities:

- D3D12 mesh shader path with feature queries and output/payload limit rows.
- Vulkan `VK_EXT_mesh_shader` path with strict toolchain/host gates.
- Fallback diagnostics when mesh shaders are unavailable.

Non-responsibilities:

- Making mesh shaders mandatory.
- Work Graphs as a default path.
- Metal readiness without Apple-host proof.

### Streaming And Residency

Future files should start in `MK_runtime` and `MK_runtime_rhi`.

Responsibilities:

- Cluster page request queue.
- Resident page catalog.
- Eviction that preserves visible fallback ancestors.
- GPU memory policy integration.

### Deformation

Future files should start in `MK_assets`, `MK_runtime_rhi`, and only touch `MK_animation` when an explicit first-party data contract is needed.

Tiers:

- Tier 0: static clusters.
- Tier 1: rigid instances.
- Tier 2: skinned clusters with conservative bone bounds.
- Tier 3: morph clusters with delta bounds.
- Tier 4: runtime displacement/destruction with bounded dynamic update policy.

Unsupported tiers must fall back to conventional rendering with diagnostics.

### Ray Tracing

Future files should start in `MK_renderer`, then move backend-local only after a focused backend plan.

Responsibilities:

- Raster/RT consistency rows derived from the same cluster graph.
- BLAS/refit/rebuild policy rows.
- Backend feature gates and update-cost evidence.

Non-responsibilities:

- Silently using unrelated coarse proxy meshes.
- Claiming cross-backend RT parity without per-backend proof.

### Editor And Tools UI

MAVG diagnostics, benchmark selection, and authoring panels must use first-party retained UI/editor rows.

Allowed:

- `MK_editor_core`
- `MK_ui`
- `MK_ui_renderer`
- private Win32/DirectWrite/TSF/UIA shell adapters

Forbidden without a new architecture decision:

- SDL3
- Dear ImGui
- Qt
- Slint
- RmlUi
- public native handles

## First Child Implementation Rule

The first implementation child after this Phase 0 baseline is:

- `docs/superpowers/plans/2026-06-05-mavg-asset-graph-v1.md`

That child must implement only deterministic asset graph and cook validation. It must not add renderer execution, streaming, mesh shaders, ray tracing, deformation, or benchmark-exceeds claims.

## Validation Gates

Docs/spec-only changes:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
git diff --check
```

Public C++ API changes:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Shader/backend changes add `tools/check-shader-toolchain.ps1`, backend-local tests, and host-gated D3D12/Vulkan/Metal evidence as applicable.
