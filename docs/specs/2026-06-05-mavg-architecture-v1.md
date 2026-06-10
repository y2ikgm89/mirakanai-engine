# 2026-06-05 MAVG Architecture v1

## Purpose

Define the clean-room architecture baseline for Mirakana Adaptive Virtual Geometry (MAVG) and the first data-only implementation boundary. This document is a specification and audit record, not a renderer/runtime ready-claim. The first implementation child adds a cooked cluster graph descriptor and package planner only; it does not add a renderer path, backend feature, streaming system, deformation system, ray tracing payload, or benchmark result.

## Status

Phase 0 specification completed for `mavg-research-legal-benchmark-baseline-v1`. The parent stacked implementation milestone is `mavg-runtime-lod-milestone-v1` over the `mavg-asset-graph-v1` foundation. The first LoD checkpoints implement deterministic `MK_assets` `GameEngine.MavgClusterGraph.v1` hierarchy/error/fallback/draw-range graph validation, `MK_tools` static `GameEngine.MavgClusterPayload.v1` vertex/index payload rows through `MavgClusterCookVertex`, `vertex.data_hex`, `index.data_hex`, per-material root/leaf fallback clusters, `MK_renderer` CPU reference selection through `mavg_lod_selection.hpp` / `select_mavg_lod_clusters`, `MK_runtime` resident-page evidence through `mavg_lod_residency.hpp` / `build_runtime_mavg_lod_residency`, `MK_runtime` caller-reviewed page request streaming planning, selected-visible/fallback-ancestor eviction protection, deterministic automatic candidate ordering through `RuntimeMavgPageStreamingAutomaticEvictionPlanDesc` / `plan_runtime_mavg_page_streaming_automatic_evictions` / `planned_automatic_eviction_policy` / `automatic_eviction_candidate_count` / `protected_eviction_candidate_skip_count`, and one-row safe-point drain through `mavg_page_streaming.hpp` / `RuntimeMavgPageStreamingPlanResult` / `RuntimeMavgPageStreamingDrainResult` / `plan_runtime_mavg_page_streaming_requests` / `review_runtime_mavg_page_streaming_evictions` / `execute_runtime_mavg_page_streaming_request_safe_point`, range-aware conventional indexed draw execution, `MK_scene_renderer` conventional `MeshCommand` planning through `mavg_scene_lod.hpp` / `plan_mavg_scene_lod_mesh_commands`, and `MK_runtime_rhi` conventional package-visible MAVG mesh binding upload evidence through `mavg_conventional_upload.hpp` / `upload_runtime_mavg_conventional_mesh_binding`. The completed prerequisite children `mavg-gpu-culling-indirect-v1`, `mavg-rhi-indirect-draw-v1`, `mavg-d3d12-indexed-indirect-draw-execution-v1`, `mavg-vulkan-indexed-indirect-draw-execution-v1`, and `mavg-d3d12-count-buffer-indirect-execution-v1` add `MK_renderer` value-only GPU culling/indirect command planning through `mavg_gpu_culling.hpp` / `plan_mavg_gpu_culling_indirect_commands`, packed indexed indirect command rows, reviewed culling bounds, fail-closed diagnostics, D3D12/Vulkan synchronization requirement rows, `MK_rhi` `indirect_draw.hpp`, `IndexedIndirectDrawCommand`, `IndexedIndirectDrawDesc`, `BufferUsage::indirect`, `IRhiCommandList::draw_indexed_indirect`, Null RHI argument/count-buffer deterministic execution counters, Vulkan indirect-buffer usage mapping, completed D3D12 `ExecuteIndirect` execution with visible WARP-backed readback proof, completed Vulkan `vkCmdDrawIndexedIndirect` execution for CPU-generated upload indexed indirect argument buffers without count buffers with SPIR-V environment-gated visible readback proof, and completed D3D12 count-buffer indirect execution through `ID3D12GraphicsCommandList::ExecuteIndirect` with a non-null count buffer and 4-byte-aligned `CountBufferOffset` executing `min(count_buffer_value, max_draw_count)` draws for CPU-generated upload argument and count buffers with visible WARP-backed `MK_d3d12_rhi_tests` count-limited/zero-count/invalid-input coverage; autonomous background workers, async-overlap/performance, runtime-inferred LRU/frequency eviction policy, partial `.mavgpayload` byte-range page loading/schema, GPU memory pressure integration, actual GPU culling dispatch, compute-generated count/argument buffers, count-buffer Vulkan execution, mesh shaders, deformation, ray tracing, Nanite equivalence/superiority, and benchmark superiority remain unclaimed until later focused tasks add code and validation evidence.

## Current Repository Baseline

- `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` selects `docs/superpowers/plans/2026-06-10-mavg-vulkan-count-buffer-indirect-execution-v1.md` with `recommendedNextPlan.id = mavg-vulkan-count-buffer-indirect-execution-v1` after MAVG D3D12 Count-Buffer Indirect Execution v1 completed through PR #547; the active child implements `vkCmdDrawIndexedIndirectCount` with shared helper reuse and SPIR-V environment-gated `MK_backend_scaffold_tests` count-limited, zero-count, and invalid-input visible readback proof.
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

- MAVG package-visible backend execution.
- Streaming IO execution.
- Runtime source import.
- Third-party simplifier ownership.
- CPU selection now belongs to the `MK_renderer` selector checkpoint, resident-page evidence belongs to the `MK_runtime` bridge checkpoint, range-aware conventional indexed draw execution belongs to the RHI/renderer checkpoint, and conventional selected-cluster `MeshCommand` planning belongs to the `MK_scene_renderer` checkpoint in the active LoD milestone.

The active detailed LoD milestone is `docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md`. Its graph, draw-ready static cook payload, CPU selector, runtime resident-page evidence, range-aware conventional indexed draw, conventional scene submission planning, conventional runtime upload evidence, docs/manifest/static sync, and slice validation checkpoints are implemented.

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
- Preserve selector `MavgLodPageRequest` rows as reviewed page requests for a later streaming plan.
- Report missing catalog/graph/catalog-kind evidence without reading package files, mutating resident mount sets, executing streaming, or touching renderer/RHI handles.

Non-responsibilities:

- Loading package files or parsing runtime sources.
- Background streaming or automatic eviction.
- Renderer/RHI upload, residency, or native handle ownership.

### Conventional Renderer Adoption

The conventional path avoids backend-specific GPU work. The selected-cluster submission planner lives in `MK_scene_renderer`; package-visible conventional upload evidence lives in `MK_runtime_rhi` and consumes already-committed package streaming/catalog state without executing streaming:

- `engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_conventional_upload.hpp`
- `engine/runtime_rhi/src/mavg_conventional_upload.cpp`
- `engine/scene_renderer/include/mirakana/scene_renderer/mavg_scene_lod.hpp`
- `engine/scene_renderer/src/mavg_scene_lod.cpp`

Implemented responsibilities:

- Expose `RuntimeMavgConventionalMeshUploadDesc`, `RuntimeMavgConventionalMeshUploadResult`, and `upload_runtime_mavg_conventional_mesh_binding` as the narrow runtime-RHI MAVG conventional upload contract.
- Validate committed package streaming evidence, live `AssetKind::mavg_cluster_graph` catalog rows, caller-owned graph documents, payload asset/handle identity, and graph draw ranges before upload side effects.
- Upload the static MAVG payload through existing `upload_runtime_mesh` and return a renderer `MeshGpuBinding` with package-visible counters, submitted fences, and explicit non-claim flags for GPU culling, indirect draws, mesh shaders, and native handles.
- Submit selected clusters through range-aware conventional `MeshCommand` planning using existing `MeshGpuBinding` and `MaterialGpuBinding` payloads.
- Preserve fallback-substitution counters and missing-material fallback-color diagnostics without exposing native handles.

Future responsibilities:

- Autonomous background/page package streaming workers, async overlap/performance evidence, partial `.mavgpayload` byte-range page loading/schema, automatic eviction policy, and GPU memory pressure integration.
- GPU culling, indirect command buffers, and mesh shader paths.

### GPU Culling And Indirect

The first value-only GPU culling planning files live in `MK_renderer`:

- `engine/renderer/include/mirakana/renderer/mavg_gpu_culling.hpp`
- `engine/renderer/src/mavg_gpu_culling.cpp`

Implemented responsibilities:

- Expose `MavgGpuCullingClusterBoundsRow`, `MavgGpuCullingIndirectCommand`, `MavgGpuCullingIndirectCommandLayout`, `MavgGpuCullingSyncRequirement`, `MavgGpuCullingIndirectDesc`, `MavgGpuCullingIndirectPlan`, and `plan_mavg_gpu_culling_indirect_commands`.
- Convert successful `MavgLodSelectionResult` rows plus reviewed culling bounds into deterministic packed indexed indirect command rows using the current conventional `draw_indexed` field order.
- Record 20-byte five-field argument layout, 4-byte count-buffer size/value, selected/visible/culled counters, fallback-substitution provenance, and fail-closed diagnostics.
- Return D3D12 and Vulkan compute-write-to-indirect-read synchronization requirement rows when the command producer is `compute_shader`.
- Keep `executed_gpu_culling`, `executed_indirect_draw`, `executed_mesh_shader`, and `touched_native_handles` false.

Future files should start in `MK_rhi`, `MK_renderer`, and backend-local implementations:

- backend-local implementation files under `engine/rhi/d3d12` and `engine/rhi/vulkan`.

The first RHI indirect draw contract files now live in `MK_rhi`:

- `engine/rhi/include/mirakana/rhi/indirect_draw.hpp`
- `engine/rhi/src/indirect_draw.cpp`
- `engine/rhi/src/null_rhi.cpp`

Implemented RHI responsibilities:

- Expose `IndexedIndirectDrawCommand`, `IndexedIndirectDrawDesc`, fixed five-word layout constants, `encode_indexed_indirect_draw_command`, and `decode_indexed_indirect_draw_command`.
- Add `BufferUsage::indirect` and `IRhiCommandList::draw_indexed_indirect` without exposing native handles.
- Let Null RHI validate argument buffer/count buffer usage, 4-byte alignment, stride, and bounds, then execute decoded commands deterministically through the existing indexed draw stats path.
- Track `indexed_indirect_draw_calls`, `indexed_indirect_commands_executed`, `indexed_indirect_count_buffer_reads`, `last_indexed_indirect_max_draw_count`, `last_indexed_indirect_executed_draw_count`, and `last_indexed_indirect_count_buffer_value`.
- Map Vulkan buffer usage planning for indirect argument buffers while leaving native Vulkan command execution to a later backend plan.

Future responsibilities:

- Actual compute culling dispatch and command/count buffer writes.
- D3D12 command signatures, resource state transitions, and `ExecuteIndirect`.
- count-buffer Vulkan execution, Vulkan indirect draw/count execution, feature gates, and synchronization commands.

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

Implemented first-child files live in `MK_runtime`:

- `engine/runtime/include/mirakana/runtime/mavg_page_streaming.hpp`
- `engine/runtime/src/mavg_page_streaming.cpp`

Implemented responsibilities:

- Expose `RuntimeMavgPageStreamingCandidateRow`, `RuntimeMavgPageStreamingPlanResult`, `RuntimeMavgPageStreamingDrainResult`, `RuntimeMavgResidentPageMountRow`, `RuntimeMavgPageStreamingSelectedClusterRow`, and `RuntimeMavgPageStreamingEvictionReviewResult` as the narrow runtime MAVG page streaming evidence rows.
- Convert reviewed selector `MavgLodPageRequest` rows into deterministic `RuntimeMavgPageStreamingPlanRow` package candidate rows.
- Skip already-resident pages, coalesce duplicate requests by highest priority, sort by priority/page, and apply deterministic `max_queued_pages` degradation.
- Convert selected cluster rows plus reviewed resident page mount rows into protected visible page and fallback ancestor mount ids, then delegate caller-reviewed candidate order to `plan_runtime_resident_package_evictions_v2` through `RuntimeMavgPageStreamingAutomaticEvictionPlanDesc`, `plan_runtime_mavg_page_streaming_automatic_evictions`, `planned_automatic_eviction_policy`, `automatic_eviction_candidate_count`, and `protected_eviction_candidate_skip_count` without runtime-inferred LRU/frequency inference.
- Expose `RuntimeMavgPageStreamingRecencyRow`, `RuntimeMavgResidentPageUseGenerationDesc`, `RuntimeMavgResidentPageUseGenerationResult`, and `infer_runtime_mavg_resident_page_use_generations` for MAVG Runtime Inferred Page Use Generation v1 side-effect-free resident page use-generation evidence without a runtime-inferred LRU/frequency eviction policy claim.
- Support caller-supplied recency ordering through `mavg-resident-page-recency-eviction-order-v1`, `RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::caller_supplied_recency`, `resident_page_last_used_generation`, `applied_caller_supplied_recency_policy`, `recency_eviction_candidate_count`, `duplicate_recency_row_count`, and `missing_recency_row_count` without runtime-inferred LRU/frequency policy, GPU memory pressure integration, DirectStorage execution, or Nanite superiority claims.
- Drain one queued row through `commit_runtime_package_candidate_resident_mount_with_reviewed_evictions_v2` at a caller-owned safe point.
- Keep zero side-effect flags for planner file IO, eviction review file IO, mount mutation, streaming execution, renderer/RHI handles, and drain background workers/RHI handles.

Future responsibilities:

- Partial `.mavgpayload` byte-range page package schema and loader.
- Autonomous/background dispatch policy and async overlap evidence.
- Automatic eviction policy beyond caller-reviewed candidate order and protected selected/fallback ancestor mount ids.
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
