# 2026-06-05 MAVG Architecture v1

## Purpose

Define the clean-room architecture baseline for Mirakana Adaptive Virtual Geometry (MAVG) and the first data-only implementation boundary. This document is a specification and audit record, not a renderer/runtime ready-claim. The first implementation child adds a cooked cluster graph descriptor and package planner only; it does not add a renderer path, backend feature, streaming system, deformation system, ray tracing payload, or benchmark result.

## Status

Phase 0 specification completed for `mavg-research-legal-benchmark-baseline-v1`. The parent stacked implementation milestone is `mavg-runtime-lod-milestone-v1` over the `mavg-asset-graph-v1` foundation. The first LoD checkpoints implement deterministic `MK_assets` `GameEngine.MavgClusterGraph.v1` hierarchy/error/fallback/draw-range graph validation, `MK_tools` static `GameEngine.MavgClusterPayload.v1` vertex/index payload rows through `MavgClusterCookVertex`, `vertex.data_hex`, `index.data_hex`, per-material root/leaf fallback clusters, `MK_renderer` CPU reference selection through `mavg_lod_selection.hpp` / `select_mavg_lod_clusters`, `MK_runtime` resident-page evidence through `mavg_lod_residency.hpp` / `build_runtime_mavg_lod_residency`, `MK_runtime` caller-reviewed page request streaming planning, selected-visible/fallback-ancestor eviction protection, deterministic automatic candidate ordering through `RuntimeMavgPageStreamingAutomaticEvictionPlanDesc` / `plan_runtime_mavg_page_streaming_automatic_evictions` / `planned_automatic_eviction_policy` / `automatic_eviction_candidate_count` / `protected_eviction_candidate_skip_count`, and one-row safe-point drain through `mavg_page_streaming.hpp` / `RuntimeMavgPageStreamingPlanResult` / `RuntimeMavgPageStreamingDrainResult` / `plan_runtime_mavg_page_streaming_requests` / `review_runtime_mavg_page_streaming_evictions` / `execute_runtime_mavg_page_streaming_request_safe_point`, range-aware conventional indexed draw execution, `MK_scene_renderer` conventional `MeshCommand` planning through `mavg_scene_lod.hpp` / `plan_mavg_scene_lod_mesh_commands`, and `MK_runtime_rhi` conventional package-visible MAVG mesh binding upload evidence through `mavg_conventional_upload.hpp` / `upload_runtime_mavg_conventional_mesh_binding`. The completed prerequisite children `mavg-gpu-culling-indirect-v1`, `mavg-rhi-indirect-draw-v1`, `mavg-d3d12-indexed-indirect-draw-execution-v1`, `mavg-vulkan-indexed-indirect-draw-execution-v1`, `mavg-d3d12-count-buffer-indirect-execution-v1`, `mavg-vulkan-count-buffer-indirect-execution-v1`, `mavg-gpu-culling-dispatch-v1`, `mavg-vulkan-gpu-culling-dispatch-v1`, and `mavg-vulkan-compute-generated-indirect-consumption-v1` add `MK_renderer` value-only GPU culling/indirect command planning through `mavg_gpu_culling.hpp` / `plan_mavg_gpu_culling_indirect_commands`, packed indexed indirect command rows, reviewed culling bounds, fail-closed diagnostics, D3D12/Vulkan synchronization requirement rows, `MK_rhi` `indirect_draw.hpp`, `IndexedIndirectDrawCommand`, `IndexedIndirectDrawDesc`, `BufferUsage::indirect`, `IRhiCommandList::draw_indexed_indirect`, Null RHI argument/count-buffer deterministic execution counters, Vulkan indirect-buffer usage mapping, completed D3D12 `ExecuteIndirect` execution with visible WARP-backed readback proof, completed Vulkan `vkCmdDrawIndexedIndirect` execution for CPU-generated upload indexed indirect argument buffers without count buffers with SPIR-V environment-gated visible readback proof, completed D3D12 count-buffer indirect execution, completed count-buffer Vulkan execution through `vkCmdDrawIndexedIndirectCount`, completed D3D12/Vulkan GPU culling dispatch, completed D3D12 compute-generated indirect consumption, completed Vulkan compute-generated indirect consumption through `is_compute_generated_indexed_indirect_buffer`, `external_argument_buffer`, `external_count_buffer`, `leave_indirect_argument_state_for_consumption`, backend-private synchronization2 barriers, and `MK_mavg_vulkan_compute_generated_indirect_consumption_tests`, completed caller-owned `.mavgpayload` page byte-range extraction through `mavg-payload-byte-range-page-loader-v1`, completed first-party filesystem-backed `.mavgpayload` page byte-range extraction through `mavg-payload-filesystem-byte-range-io-v1`, first-party background worker dispatch for reviewed package candidate loads through `mavg-background-streaming-dispatch-v1`, and caller-owned persistent pending-row service ticks through `mavg-persistent-background-streaming-service-v1`; autonomous background streaming services, async-overlap/performance proof, runtime-inferred LRU/frequency eviction policy, DirectStorage execution, GPU memory pressure integration, package-visible MAVG backend readiness, mesh shaders, deformation, ray tracing, Nanite equivalence/superiority, benchmark superiority, and broad optimization remain unclaimed until later focused tasks add code and validation evidence.

## Current Repository Baseline

- `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` selects `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md` with `recommendedNextPlan.id = next-production-gap-selection` after MAVG Vulkan Compute-Generated Indirect Consumption v1 (`mavg-vulkan-compute-generated-indirect-consumption-v1`) completed through PR #567 / merge commit `9c6b681f`, MAVG Vulkan GPU Culling Dispatch v1 completed through PR #563 (merge commit `5dce6857`), and MAVG GPU Culling Dispatch v1 completed through PR #556 and closeout PR #557; completed `dispatch_mavg_gpu_culling_indirect` with WARP-backed `MK_mavg_gpu_culling_dispatch_tests`, `VK_KHR_synchronization2` buffer_memory_barrier2 rows, SPIR-V environment-gated `MK_mavg_vulkan_gpu_culling_dispatch_tests`, and `MK_mavg_vulkan_compute_generated_indirect_consumption_tests` evidence remains authoritative. Vulkan `draw_indexed_indirect` consumes MAVG compute-generated argument/count buffers through `is_compute_generated_indexed_indirect_buffer`, `external_argument_buffer`, `external_count_buffer`, `leave_indirect_argument_state_for_consumption`, and backend-private synchronization2 barriers while the CPU-upload `copy_source` path remains unchanged. MAVG D3D12 Compute-Generated Indirect Consumption v1 (`mavg-d3d12-compute-generated-indirect-consumption-v1`) completed through PR #560 over activation commit `d9327b69`: D3D12 `draw_indexed_indirect` consumes GPU-written MAVG packed argument and count buffers from `dispatch_mavg_gpu_culling_indirect` through `is_compute_generated_indexed_indirect_buffer`, backend-private UAV-to-indirect-argument transitions, `native_buffer_resource`, `leave_indirect_argument_state_for_consumption`, and WARP-backed `MK_mavg_d3d12_compute_generated_indirect_consumption_tests` while the CPU-upload `copy_source` path remains unchanged.
- MAVG Payload Byte-Range Page Loader v1 (`mavg-payload-byte-range-page-loader-v1`) adds `mavg_cluster_payload_format_v1`, graph `invalid_page_byte_range` validation, `mavg_payload_page_loader.hpp`, `RuntimeMavgPayloadPageLoadDesc`, `RuntimeMavgPayloadPageLoadResult`, `RuntimeMavgPayloadPageRow`, and `load_runtime_mavg_payload_pages` for side-effect-free extraction of reviewed `.mavgpayload` page byte ranges from caller-owned payload bytes. `MK_runtime_mavg_payload_page_loader_tests` and `MK_mavg_cluster_graph_tests` prove exact range extraction and fail-closed diagnostics without file IO, background streaming workers, DirectStorage, GPU memory policy, renderer/RHI handles, or package-visible backend readiness claims.
- MAVG Payload Filesystem Byte-Range IO v1 (`mavg-payload-filesystem-byte-range-io-v1`) adds `IFileSystem::read_binary_range`, `MemoryFileSystem::read_binary_range`, `RootedFileSystem::read_binary_range`, `RuntimeMavgPayloadFilesystemPageLoadDesc`, and `load_runtime_mavg_payload_pages_from_filesystem` for synchronous first-party filesystem reads of the reviewed payload format prefix and requested page byte ranges only. `MK_core_tests` and `MK_runtime_mavg_payload_page_loader_tests` prove exact range extraction, request-order preservation, out-of-bounds rejection, no text read fallback, and no mount mutation / background worker / DirectStorage / GPU memory policy / renderer-RHI side effects.
- MAVG DirectStorage Page IO Execution v1 (`mavg-directstorage-page-io-execution-v1`) adds `IByteRangeIoExecutor`, `ByteRangeIoBackendKind::direct_storage`, `RuntimeMavgPayloadDirectStoragePageLoadDesc`, and `load_runtime_mavg_payload_pages_from_direct_storage` so reviewed `.mavgpayload` page ranges can execute through a caller-owned DirectStorage byte-range executor while failing closed without filesystem fallback or first-party Win32 DirectStorage SDK adapter claims.
- MAVG Background Streaming Dispatch v1 (`mavg-background-streaming-dispatch-v1`) adds `RuntimeMavgPageStreamingBackgroundLoadDesc`, `RuntimeMavgPageStreamingBackgroundLoadedRow`, `RuntimeMavgPageStreamingBackgroundLoadResult`, and `dispatch_runtime_mavg_page_streaming_background_loads` for dispatching reviewed package candidate loads onto caller-owned first-party `JobExecutionPool` workers. MAVG Persistent Background Streaming Service v1 (`mavg-persistent-background-streaming-service-v1`) adds `RuntimeMavgPageStreamingBackgroundServiceState`, `RuntimeMavgPageStreamingBackgroundServiceTickResult`, and `tick_runtime_mavg_page_streaming_background_service` for caller-owned pending-row retention, duplicate coalescing, and bounded background dispatch across ticks without owning autonomous scheduling. `MK_runtime_mavg_page_streaming_tests` proves worker dispatch, persistent pending-row service dispatch, package load failure diagnostics, deterministic loaded-row order, and no resident mount mutation / catalog refresh / DirectStorage / GPU memory policy / renderer-RHI side effects.
- MAVG GPU Memory Pressure Residency v1 (`mavg-gpu-memory-pressure-residency-v1`) adds `mavg_gpu_memory_residency.hpp`, `RuntimeMavgGpuMemoryResidencyDesc`, `RuntimeMavgGpuMemoryResidencyResult`, and `plan_runtime_mavg_gpu_memory_pressure_residency` in `MK_runtime_rhi` so a successful renderer `GpuMemoryPolicyPlan` counted-byte target becomes `RuntimeResourceResidencyBudgetV2::max_resident_content_bytes` for selected/fallback-protected MAVG automatic eviction planning. `MK_runtime_rhi_mavg_gpu_memory_residency_tests` proves recency-ordered eviction planning and fail-closed missing residency pressure evidence without live mount mutation, catalog refresh, file IO, DirectStorage, async-overlap/performance proof, renderer/RHI handle access, or GPU upload.
- MAVG Cluster Streaming Residency Closeout v1 (`mavg-cluster-streaming-residency-closeout-v1`) adds `mavg_cluster_streaming_residency_closeout.hpp`, `RuntimeMavgClusterStreamingResidencyCloseoutResult`, and `plan_runtime_mavg_cluster_streaming_residency_closeout` in `MK_runtime_rhi` to compose reviewed page planning, `load_runtime_mavg_payload_pages_from_filesystem`, `dispatch_runtime_mavg_page_streaming_background_loads`, and `plan_runtime_mavg_gpu_memory_pressure_residency` into one value-level Phase 5 closeout with deterministic degradation counters. It does not claim live mount mutation, catalog refresh, DirectStorage, GPU upload, backend execution, mesh shaders, Nanite readiness, or broad optimization.
- MAVG Cluster Streaming Safe Point Adoption v1 (`mavg-cluster-streaming-safe-point-adoption-v1`) adds `mavg_cluster_streaming_safe_point_adoption.hpp`, `RuntimeMavgClusterStreamingSafePointAdoptionResult`, and `execute_runtime_mavg_cluster_streaming_safe_point_adoption` in `MK_runtime_rhi` so successful closeout `background_load.loaded_rows` can be projected into caller-owned `RuntimeResidentPackageMountSetV2` mounts, reviewed through `plan_runtime_resident_package_evictions_v2`, refreshed into `RuntimeResidentCatalogCacheV2`, and committed only after projection succeeds. It does not claim package candidate reload, persistent/autonomous streaming services, DirectStorage, GPU upload, backend execution, renderer/RHI handle access, mesh shaders, Nanite readiness, or broad optimization.
- MAVG Streamed Cluster GPU Upload v1 (`mavg-streamed-cluster-gpu-upload-v1`) adds `mavg_streamed_cluster_gpu_upload.hpp`, `RuntimeMavgStreamedClusterGpuUploadResult`, and `upload_runtime_mavg_streamed_cluster_pages` in `MK_runtime_rhi` so committed `RuntimeMavgClusterStreamingSafePointAdoptionResult` rows can be validated against `RuntimeResidentCatalogCacheV2` mesh residency and caller-owned page payloads before publishing page-level `MeshGpuBinding` rows through existing runtime mesh upload. It does not claim package candidate reload, persistent/autonomous streaming services, DirectStorage, backend execution, mesh shader execution, renderer/RHI native handles, async-overlap/performance proof, Nanite readiness, or broad optimization.
- MAVG Mesh Shader Capability Planner v1 (`mavg-mesh-shader-capability-planner-v1`) adds `mavg_mesh_shader_policy.hpp`, `MavgMeshShaderCapabilityDesc`, `MavgMeshShaderCapabilityPlan`, and `plan_mavg_mesh_shader_capability` in `MK_renderer` so caller-reviewed D3D12/Vulkan capability rows can produce deterministic mesh-shader-ready or compute/indirect fallback rows with output/payload/threadgroup/combined-payload-output-memory/pipeline-statistics diagnostics. `MK_mavg_mesh_shader_policy_tests` proves reviewed-support selection, fallback on missing backend support, insufficient-limit diagnostics, D3D12 pipeline-statistics advisory diagnostics, deterministic missing-backend rows, and zero mesh shader execution / native-handle access.
- MAVG Streamed Cluster Backend Draw Execution v1 (`mavg-streamed-cluster-backend-draw-execution-v1`) adds `RuntimeMavgStreamedSceneLodSubmitDesc` and `plan_runtime_mavg_streamed_scene_lod_mesh_commands` in `MK_runtime_scene_rhi` so ready streamed page `MeshGpuBinding` rows from `RuntimeMavgStreamedClusterGpuUploadResult` become page-asset `MeshCommand` rows with graph-authored `MeshIndexedDrawRange` values and focused `NullRenderer` / `RhiFrameRenderer` evidence. It does not claim package candidate reload, persistent/autonomous streaming services, DirectStorage, mesh shader execution, renderer/RHI native handles beyond existing first-party handles in binding rows, async-overlap/performance proof, Nanite readiness, or broad optimization.
- MAVG Mesh Shader Capability Planner v1 (`mavg-mesh-shader-capability-planner-v1`) adds `mavg_mesh_shader_policy.hpp`, `MavgMeshShaderCapabilityDesc`, `MavgMeshShaderCapabilityPlan`, and `plan_mavg_mesh_shader_capability` in `MK_renderer` so caller-reviewed D3D12/Vulkan capability rows can produce deterministic mesh-shader-ready or compute/indirect fallback rows with output/payload/threadgroup/combined-payload-output-memory/pipeline-statistics diagnostics. `MK_mavg_mesh_shader_policy_tests` proves reviewed-support selection, fallback on missing backend support, insufficient-limit diagnostics, D3D12 pipeline-statistics advisory diagnostics, deterministic missing-backend rows, and zero mesh shader execution / native-handle access.
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

- Persistent/autonomous background package streaming services, async overlap/performance proof, DirectStorage execution, backend draw execution, mesh shader execution, and broad GPU memory pressure enforcement beyond value-only resident byte-budget eviction planning.
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
- Keep `executed_gpu_culling`, `executed_indirect_draw`, `executed_mesh_shader`, and `touched_native_handles` false in the CPU-reference planner path.

Implemented D3D12 GPU culling dispatch files now live in `MK_rhi_d3d12` and `MK_renderer`:

- `engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_mavg_gpu_culling_dispatch.hpp`
- `engine/rhi/d3d12/src/d3d12_mavg_gpu_culling_dispatch.cpp`
- `build_mavg_gpu_culling_dispatch_cluster_rows`, `encode_mavg_gpu_culling_indirect_argument_buffer_bytes`, and `encode_mavg_gpu_culling_indirect_count_buffer_bytes` in `mavg_gpu_culling.hpp`

Implemented D3D12 dispatch responsibilities:

- Execute `dispatch_mavg_gpu_culling_indirect` from reviewed cluster rows and write packed indexed indirect argument/count buffer bytes through a D3D12 compute dispatch.
- Record compute-write-to-indirect-read synchronization through D3D12 UAV barriers and `D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT` transitions without public buffer-state tracking.
- Set `executed_gpu_culling=true` only after successful dispatch and prove visible/culled cases through WARP-backed `MK_mavg_gpu_culling_dispatch_tests`.

Future files should start in `MK_rhi`, `MK_renderer`, and backend-local implementations:

- backend-local Vulkan compute dispatch under `engine/rhi/vulkan`.

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

- Package-visible MAVG backend readiness and broader integration over the selected D3D12/Vulkan compute-generated indirect paths.
- Mesh-shader, Work Graphs, Metal, ray tracing, deformation, streaming, residency, and benchmark evidence.

### Mesh Shader Backends

Current value-only capability files start in `MK_renderer`; future execution files should start in `shaders/` and backend-local `MK_rhi_*` code.

Responsibilities:

- Represent caller-reviewed D3D12/Vulkan mesh shader support, task/amplification support, output/payload/threadgroup/combined-payload-output-memory limits, and D3D12 pipeline-statistics availability as first-party rows.
- Select compute/indirect fallback when reviewed support or limits are insufficient.
- D3D12 mesh shader path with feature queries and output/payload limit rows.
- Vulkan `VK_EXT_mesh_shader` path with strict toolchain/host gates.
- Fallback diagnostics when mesh shaders are unavailable.

Non-responsibilities:

- Making mesh shaders mandatory.
- Executing `DispatchMesh` or `vkCmdDrawMeshTasksEXT` from the value-only policy.
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
- Support caller-supplied recency ordering through `mavg-resident-page-recency-eviction-order-v1`, `RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::caller_supplied_recency`, `resident_page_last_used_generation`, `applied_caller_supplied_recency_policy`, `recency_eviction_candidate_count`, `duplicate_recency_row_count`, and `missing_recency_row_count` without runtime-inferred LRU/frequency policy, DirectStorage execution, or Nanite superiority claims. MAVG Persistent Background Streaming Service v1 adds `RuntimeMavgPageStreamingBackgroundServiceState`, `RuntimeMavgPageStreamingBackgroundServiceTickResult`, and `tick_runtime_mavg_page_streaming_background_service` so caller-owned state can retain reviewed rows across frames, coalesce duplicate pending pages, and dispatch bounded background package loads while still returning loaded rows for later caller-owned safe-point adoption. MAVG GPU Memory Pressure Residency v1 can feed the existing selected/fallback-protected eviction planner with a reviewed resident content byte budget derived from successful renderer `GpuMemoryPolicyPlan` evidence, and MAVG Cluster Streaming Residency Closeout v1 composes that with reviewed byte-range payload IO and background package candidate loads into deterministic degradation evidence. MAVG Cluster Streaming Safe Point Adoption v1 then commits only the successful closeout `background_load.loaded_rows` into caller-owned `RuntimeResidentPackageMountSetV2` mounts and `RuntimeResidentCatalogCacheV2` through `plan_runtime_resident_package_evictions_v2` after projection succeeds, without reloading candidates, DirectStorage, GPU upload, backend execution, renderer/RHI handle access, or broad optimization.
- Drain one queued row through `commit_runtime_package_candidate_resident_mount_with_reviewed_evictions_v2` at a caller-owned safe point.
- Keep zero side-effect flags for planner file IO, eviction review file IO, mount mutation, streaming execution, renderer/RHI handles, and drain background workers/RHI handles.

Future responsibilities:

- Autonomous filesystem-to-resident streaming orchestration after the synchronous filesystem byte-range loader.
- Autonomous service scheduling/backpressure/cancellation and async overlap evidence beyond the caller-owned persistent tick.
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
