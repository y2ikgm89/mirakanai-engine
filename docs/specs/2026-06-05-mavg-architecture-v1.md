# 2026-06-05 MAVG Architecture v1

## Purpose

Define the clean-room architecture baseline for Mirakana Adaptive Virtual Geometry (MAVG) and the current data-only/runtime-evidence implementation boundary. This document is a specification and audit record, not a broad renderer/runtime ready-claim. The stacked implementation children add reviewed graph, payload, selection, page streaming planning, deterministic automatic eviction candidate ordering, caller-supplied recency ordering, value-only runtime-inferred page-use generation evidence, runtime-inferred LRU eviction ordering, a narrow joined engine-owned page streaming worker, conventional upload, and selected D3D12 proof rows only; they do not add DirectStorage native queue execution, async-overlap/performance claims, broad backend parity, deformation, ray tracing payloads, or benchmark superiority.

## Status

Phase 0 specification completed for `mavg-research-legal-benchmark-baseline-v1`. The parent stacked implementation milestone `mavg-runtime-lod-milestone-v1` over the `mavg-asset-graph-v1` foundation is closed, and the active pointer now returns to the production master plan with `recommendedNextPlan.id = next-production-gap-selection` after MAVG Autonomous Page Streaming Worker v1 (`mavg-autonomous-page-streaming-worker-v1`) completed through stacked draft PR #471, follow-up MAVG Automatic Eviction Policy v1 (`mavg-automatic-eviction-policy-v1`) added deterministic resident-page eviction candidate ordering, follow-up MAVG Resident Page Recency Eviction Order v1 (`mavg-resident-page-recency-eviction-order-v1`) added caller-supplied recency ordering, follow-up MAVG Runtime-Inferred Page Use Generation v1 (`mavg-runtime-inferred-page-use-generation-v1`) added selected resident page use-generation evidence, follow-up MAVG Runtime-Inferred LRU Eviction Policy v1 (`mavg-runtime-inferred-lru-eviction-policy-v1`) added explicit runtime-inferred LRU ordering, `mavg-directstorage-sdk-dependency-gate-v1` completed through draft PR #469, `mavg-win32-iocp-file-io-worker-v1` completed through draft PR #466, `mavg-win32-async-file-io-adapter-v1` completed through draft PR #463, `mavg-native-directstorage-win32-async-io-dispatch-status-v1` completed through draft PR #462, and `mavg-directstorage-request-plan-v1` completed as draft PR #460. The LoD checkpoints implement deterministic graph/payload/selection/page-streaming/conventional-renderer evidence, DirectStorage-shaped request planning through `RuntimeMavgPayloadDirectStorageRequestPlanResult`, caller-owned handle-free adapter/status APIs through `IRuntimeMavgPayloadNativeIoDispatcher`, `RuntimeMavgPayloadNativeIoDispatchResult`, `RuntimeMavgPayloadNativeIoStatusPollResult`, `dispatch_runtime_mavg_payload_native_io_requests`, and `poll_runtime_mavg_payload_native_io_status`, deterministic automatic eviction candidate ordering through `RuntimeMavgPageStreamingAutomaticEvictionPlanDesc`, `plan_runtime_mavg_page_streaming_automatic_evictions`, `planned_automatic_eviction_policy`, `automatic_eviction_candidate_count`, and `protected_eviction_candidate_skip_count`, caller-supplied recency ordering through `RuntimeMavgPageStreamingRecencyRow`, `RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::caller_supplied_recency`, `resident_page_last_used_generation`, `applied_caller_supplied_recency_policy`, `recency_eviction_candidate_count`, `duplicate_recency_row_count`, and `missing_recency_row_count`, runtime-inferred page-use generation evidence through `RuntimeMavgResidentPageUseGenerationDesc`, `RuntimeMavgResidentPageUseGenerationResult`, `infer_runtime_mavg_resident_page_use_generations`, `inferred_resident_page_use_generation`, `touched_resident_page_count`, `carried_recency_row_count`, `new_resident_page_count`, `dropped_nonresident_recency_row_count`, and `non_monotonic_use_generation`, runtime-inferred LRU eviction ordering through `RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::runtime_inferred_lru`, `inferred_eviction_policy`, `inferred_lru_eviction_policy`, and `runtime_inferred_lru_eviction_candidate_count`, a caller-polled Win32 overlapped file-read adapter through `Win32MavgPayloadAsyncFileIoDispatcher`, a completed IOCP worker adapter through `Win32MavgPayloadIocpFileIoDispatcher`, `CreateIoCompletionPort`, `GetQueuedCompletionStatus`, `PostQueuedCompletionStatus`, `CreateFileW`, `ReadFile`, `OVERLAPPED`, `CancelIoEx`, and `executed_background_worker=true`, a completed DirectStorage SDK Dependency Gate v1 through `directstorage-sdk`, `dstorage`, `MK_ENABLE_DIRECTSTORAGE_SDK`, `tools/validate-directstorage-sdk.ps1`, `MK_runtime_host_win32_directstorage_sdk_tests`, `dstorage.h`, `dstorageerr.h`, and `Microsoft::DirectStorage`, and a completed narrow autonomous background engine-owned page streaming worker through `RuntimeMavgPageStreamingDispatchMode::engine_owned_background_worker`, `RuntimeMavgPageStreamingWorkerDesc`, `RuntimeMavgPageStreamingWorkerResult`, and `execute_runtime_mavg_page_streaming_worker`. Concrete DirectStorage native queue execution, `IDStorageFactory`, `IDStorageQueue`, `IDStorageStatusArray`, `IDStorageQueue3`, `ID3D12Fence`, DirectStorage file IO execution, sustained async-overlap/performance, runtime-inferred frequency eviction policy, GPU memory pressure integration, generic GPU culling frameworks beyond the selected D3D12 proof, Vulkan indirect draw execution, mesh shaders, deformation, ray tracing, Metal readiness, Nanite equivalence/superiority, and benchmark superiority remain unclaimed until later focused tasks add code and validation evidence.

## Current Repository Baseline

- `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` selects `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md` after MAVG Autonomous Page Streaming Worker v1 closed the parent `docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md` through stacked draft PR #471.
- Completed child `mavg-autonomous-page-streaming-worker-v1` evidence is limited to autonomous background execution through `RuntimeMavgPageStreamingDispatchMode::engine_owned_background_worker`, `RuntimeMavgPageStreamingWorkerDesc`, `RuntimeMavgPageStreamingWorkerResult`, and `execute_runtime_mavg_page_streaming_worker` over already-reviewed dispatch rows and safe-point drains; it starts and joins a private worker thread before returning deterministic evidence and does not execute DirectStorage file IO, infer eviction policy, touch renderer/RHI handles, or claim async-overlap/performance or Nanite readiness.
- Follow-up child `mavg-automatic-eviction-policy-v1` evidence is limited to `RuntimeMavgPageStreamingAutomaticEvictionPlanDesc`, `plan_runtime_mavg_page_streaming_automatic_evictions`, `planned_automatic_eviction_policy`, `automatic_eviction_candidate_count`, and `protected_eviction_candidate_skip_count`; it reuses reviewed selected visible/fallback ancestor protection, skips protected resident page mounts, orders remaining resident page eviction candidates deterministically, and does not infer LRU/recency/frequency behavior, touch renderer/RHI handles, mutate live mounts, execute DirectStorage file IO, or claim async-overlap/performance or Nanite readiness.
- Follow-up child `mavg-resident-page-recency-eviction-order-v1` evidence is limited to `RuntimeMavgPageStreamingRecencyRow`, `RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::caller_supplied_recency`, `resident_page_last_used_generation`, `applied_caller_supplied_recency_policy`, `recency_eviction_candidate_count`, `duplicate_recency_row_count`, and `missing_recency_row_count`; it orders unprotected resident eviction candidates by reviewed caller-supplied last-use generation evidence and does not infer runtime LRU/frequency behavior, touch renderer/RHI handles, mutate live mounts, execute DirectStorage file IO, integrate GPU memory pressure, or claim async-overlap/performance or Nanite readiness.
- Follow-up child `mavg-runtime-inferred-page-use-generation-v1` evidence is limited to `RuntimeMavgResidentPageUseGenerationDesc`, `RuntimeMavgResidentPageUseGenerationResult`, `infer_runtime_mavg_resident_page_use_generations`, `inferred_resident_page_use_generation`, `touched_resident_page_count`, `carried_recency_row_count`, `new_resident_page_count`, `dropped_nonresident_recency_row_count`, and `non_monotonic_use_generation`; it derives reviewed `RuntimeMavgPageStreamingRecencyRow` state from current selected resident pages for the existing caller-supplied recency policy and does not infer a runtime frequency eviction policy, touch renderer/RHI handles, mutate live mounts, execute DirectStorage file IO, integrate GPU memory pressure, or claim async-overlap/performance or Nanite readiness.
- Follow-up child `mavg-runtime-inferred-lru-eviction-policy-v1` evidence is limited to `RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::runtime_inferred_lru`, `previous_recency_rows`, `current_use_generation`, `inferred_eviction_policy`, `inferred_lru_eviction_policy`, and `runtime_inferred_lru_eviction_candidate_count`; it routes explicit runtime-inferred LRU ordering through `infer_runtime_mavg_resident_page_use_generations`, preserves selected visible/fallback ancestor protection, orders only unprotected resident eviction candidates by older `resident_page_last_used_generation` first, and does not infer frequency behavior, touch renderer/RHI handles, mutate live mounts, execute DirectStorage file IO, integrate GPU memory pressure, or claim async-overlap/performance or Nanite readiness.
- Completed child `mavg-directstorage-sdk-dependency-gate-v1` / DirectStorage SDK Dependency Gate v1 evidence is limited to the optional `directstorage-sdk` vcpkg feature, `dstorage`, `MK_ENABLE_DIRECTSTORAGE_SDK`, `directstorage-sdk` presets, `tools/validate-directstorage-sdk.ps1`, `MK_runtime_host_win32_directstorage_sdk_tests`, `dstorage.h`, `dstorageerr.h`, `Microsoft::DirectStorage`, DirectStorage runtime DLL copy wiring, and draft PR #469; it does not call `DStorageGetFactory`, create DirectStorage factories/queues/status arrays, use `ID3D12Fence`, run DirectStorage file IO, or claim async-overlap/performance or Nanite readiness. The optional DirectStorage SDK lane still needs an approval-capable `tools/bootstrap-deps.ps1` run to install `dstorage` before `tools/validate-directstorage-sdk.ps1` can pass.
- Completed child `mavg-win32-async-file-io-adapter-v1` evidence remains retained through `RuntimeMavgPayloadDirectStorageRequestRow::source_file_path`, `destination_memory`, `Win32MavgPayloadAsyncFileIoDispatcherDesc`, `Win32MavgPayloadAsyncFileIoDispatcher`, `CreateFileW`, `FILE_FLAG_OVERLAPPED`, `OVERLAPPED`, `CreateEventW`, `ReadFile`, `GetOverlappedResult`, `ERROR_IO_PENDING`, `ERROR_IO_INCOMPLETE`, `CancelIoEx`, draft PR #463, no `dstorage.h` compile path, no DirectStorage SDK dependency, no IOCP worker in that adapter, no async-overlap/performance claim, and no Nanite claim.
- Completed child `mavg-native-directstorage-win32-async-io-dispatch-status-v1` evidence remains retained through `RuntimeMavgPayloadNativeIoBackend`, `IRuntimeMavgPayloadNativeIoDispatcher`, `RuntimeMavgPayloadNativeIoDispatchResult`, `RuntimeMavgPayloadNativeIoStatusPollResult`, `dispatch_runtime_mavg_payload_native_io_requests`, and `poll_runtime_mavg_payload_native_io_status` with no `dstorage.h` compile path, no DirectStorage SDK dependency, no async-overlap/performance claim, and no Nanite claim.
- Completed prerequisite `mavg-directstorage-request-plan-v1` evidence remains retained through `RuntimeMavgPayloadDirectStorageRequestPlanDesc`, `RuntimeMavgPayloadDirectStorageRequestRow`, `RuntimeMavgPayloadDirectStorageRequestPlanResult`, `RuntimeMavgPayloadDirectStorageFenceWaitPoint`, and `plan_runtime_mavg_payload_directstorage_requests` with `requires_native_directstorage_sdk=true`, `used_native_directstorage=false`, no async-overlap/performance claim, and no Nanite claim.
- Completed prerequisite `mavg-package-streaming-residency-dispatch-v1` evidence remains retained through `RuntimeMavgPageStreamingDispatchPlan`, `plan_runtime_mavg_page_streaming_dispatches`, `RuntimeMavgPageStreamingDrainDesc`, caller-owned safe-point/background rows, and explicit non-claims for sustained background streaming services, async overlap/performance, partial native byte-range streaming execution, automatic eviction policy, GPU memory pressure integration, and Nanite equivalence/superiority.
- Completed prerequisite `mavg-page-addressable-payload-schema-v1` evidence remains retained through `mavg_cluster_payload.hpp`, `MavgClusterPayloadDocument`, `MavgClusterPayloadPage`, `validate_mavg_cluster_payload`, `page.data_hex`, `mavg_payload_pages.hpp`, `RuntimeMavgPayloadPageSliceResult`, and `extract_runtime_mavg_payload_page_slices` without DirectStorage/Win32 async IO, sustained background streaming services, async overlap/performance, automatic eviction policy, GPU memory pressure integration, or Nanite claims.
- Completed prerequisite `mavg-payload-byte-range-file-io-v1` evidence remains retained through `IFileSystem read_bytes`, `write_bytes`, `read_byte_range`, `RuntimeMavgPayloadPageFileLoadResult`, and `load_runtime_mavg_payload_file_pages` with `used_native_directstorage=false`, no native DirectStorage queues/fences/status arrays, no async-overlap/performance claim, and no Nanite claim.
- Completed GPU/RHI prerequisites remain retained: `mavg_gpu_culling.hpp` exposes value-only `MavgGpuCullingIndirectPlan` / `plan_mavg_gpu_culling_indirect_commands` rows with D3D12/Vulkan sync intent; `mavg-rhi-indirect-draw-v1` adds `indirect_draw.hpp`, `IndexedIndirectDrawDesc`, `BufferUsage::indirect`, and Null RHI execution; `mavg-d3d12-indexed-indirect-draw-execution-v1` covers CPU-generated upload argument-buffer `ExecuteIndirect`; `mavg-d3d12-indexed-indirect-count-buffer-execution-v1` covers CPU-generated upload count-buffer execution with zero-count and clamp evidence; `mavg-d3d12-compute-generated-indirect-execution-v1` covers compute-generated `BufferUsage::storage | BufferUsage::indirect` buffers, `D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT`, WARP-backed fail-closed proof, and no CPU decode/stat overclaim; `mavg-d3d12-gpu-culling-execution-v1` covers the selected D3D12 MAVG GPU culling dispatch into indexed indirect buffers with visible/culled WARP-backed evidence while generic GPU culling frameworks, generic GPU-generated count-buffer systems, generic storage-buffer state management, Vulkan indirect draw execution, Metal, mesh shaders, Nanite compatibility/equivalence/superiority, and broad optimization remain unclaimed.
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

- Broader sustained background/page package streaming workers beyond the joined engine-owned proof, async overlap/performance evidence, DirectStorage native file IO execution, runtime-inferred frequency eviction policy, and GPU memory pressure integration.
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
- `engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp`
- `engine/rhi/d3d12/src/d3d12_backend.cpp`

Implemented RHI responsibilities:

- Expose `IndexedIndirectDrawCommand`, `IndexedIndirectDrawDesc`, fixed five-word layout constants, `encode_indexed_indirect_draw_command`, and `decode_indexed_indirect_draw_command`.
- Add `BufferUsage::indirect` and `IRhiCommandList::draw_indexed_indirect` without exposing native handles.
- Let Null RHI validate argument buffer/count buffer usage, 4-byte alignment, stride, and bounds, then execute decoded commands deterministically through the existing indexed draw stats path.
- Track `indexed_indirect_draw_calls`, `indexed_indirect_commands_executed`, `indexed_indirect_count_buffer_reads`, `last_indexed_indirect_max_draw_count`, `last_indexed_indirect_executed_draw_count`, and `last_indexed_indirect_count_buffer_value`.
- Map Vulkan buffer usage planning for indirect argument buffers while leaving native Vulkan command execution to a later backend plan.
- Execute D3D12 indexed indirect draws with optional CPU-generated upload count buffers through backend-private `D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED` command signatures and `ID3D12GraphicsCommandList::ExecuteIndirect` when the argument/count buffers use `BufferUsage::indirect | BufferUsage::copy_source`.
- Execute a narrow D3D12 compute-generated indexed indirect argument/count buffer lane when the same default-heap buffer uses `BufferUsage::storage | BufferUsage::indirect`; the D3D12 backend tracks the storage UAV write mark, records a UAV barrier, transitions the buffer to `D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT`, and does not CPU-decode GPU-generated command bytes for stats.
- Execute the selected D3D12 MAVG GPU culling proof by using `MavgGpuCullingIndirectPlan` as the CPU reference expectation, compacting selected-cluster command rows plus visibility rows in a compute shader, and executing the visible compacted result through the existing same-buffer argument/count `ExecuteIndirect` path.
- Keep generic GPU culling frameworks, generic GPU-generated count-buffer systems beyond this D3D12 storage-indirect lane, generic storage-buffer state management, public buffer-state tracking, and Vulkan/Metal backend execution fail-closed until explicit follow-up plans add feature gates and synchronization proof.

Future responsibilities:

- Broader GPU culling framework work beyond the selected D3D12 MAVG visibility compaction proof.
- Vulkan indirect draw/count execution, feature gates, and synchronization commands.

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

Implemented streaming and payload files live in `MK_assets` and `MK_runtime`:

- `engine/runtime/include/mirakana/runtime/mavg_page_streaming.hpp`
- `engine/runtime/src/mavg_page_streaming.cpp`
- `engine/assets/include/mirakana/assets/mavg_cluster_payload.hpp`
- `engine/assets/src/mavg_cluster_payload.cpp`
- `engine/runtime/include/mirakana/runtime/mavg_payload_pages.hpp`
- `engine/runtime/src/mavg_payload_pages.cpp`

Implemented responsibilities:

- Expose `RuntimeMavgPageStreamingCandidateRow`, `RuntimeMavgPageStreamingPlanResult`, `RuntimeMavgPageStreamingDrainDesc`, `RuntimeMavgPageStreamingDrainResult`, `RuntimeMavgResidentPageMountRow`, `RuntimeMavgPageStreamingSelectedClusterRow`, `RuntimeMavgPageStreamingEvictionReviewResult`, `review_runtime_mavg_page_streaming_evictions`, `RuntimeMavgPageStreamingDispatchPlan`, `plan_runtime_mavg_page_streaming_dispatches`, `RuntimeMavgPageStreamingWorkerDesc`, `RuntimeMavgPageStreamingWorkerResult`, `execute_runtime_mavg_page_streaming_worker`, and `execute_runtime_mavg_page_streaming_request_safe_point` as the narrow runtime MAVG page streaming evidence rows.
- Expose `MavgClusterPayloadDocument`, `MavgClusterPayloadPage`, `MavgClusterPayloadCluster`, `mavg_cluster_payload_format_v1`, and `validate_mavg_cluster_payload` as the narrow `GameEngine.MavgClusterPayload.v1` page-addressable payload schema.
- Serialize cooked `page.data_hex` logical page bytes, page table rows, and cluster page rows deterministically from the graph page rows.
- Validate graph asset match, graph validity, index format, vertex/index/page hex sizes, duplicate/missing/unknown pages, byte-range overflow/out-of-bounds, overlapping page ranges, missing/duplicate/unknown clusters, cluster page mismatch, and cluster draw-range mismatch.
- Expose `RuntimeMavgPayloadPageSliceResult` and `extract_runtime_mavg_payload_page_slices` so caller-supplied payload text plus a validated graph can return selected decoded `payload_bytes` in request order without file IO. Expose `RuntimeMavgPayloadPageFileLoadResult` and `load_runtime_mavg_payload_file_pages` so caller-supplied blob paths can read selected validated payload page `byte_offset` / `byte_size` ranges through `IFileSystem::read_byte_range` with `used_native_directstorage=false`.
- Convert reviewed selector `MavgLodPageRequest` rows into deterministic `RuntimeMavgPageStreamingPlanRow` package candidate rows.
- Skip already-resident pages, coalesce duplicate requests by highest priority, sort by priority/page, and apply deterministic `max_queued_pages` degradation.
- Convert selected cluster rows plus reviewed resident page mount rows into protected visible page and fallback ancestor mount ids, then delegate caller-reviewed candidate order to `plan_runtime_resident_package_evictions_v2` without inferring an eviction policy.
- Convert already-reviewed queued page rows plus caller-assigned mount ids, budget, overlay, and reviewed eviction/protection rows into deterministic caller-owned safe-point/background-queue dispatch rows through `plan_runtime_mavg_page_streaming_dispatches`.
- Drain one queued row through `commit_runtime_package_candidate_resident_mount_with_reviewed_evictions_v2` at a caller-owned safe point.
- Keep zero side-effect flags for planner file IO, eviction review file IO, dispatch planner file IO, mount mutation, streaming execution, renderer/RHI handles, and drain/background worker execution.

Future responsibilities:

- Native DirectStorage SDK queue, fence, and status-array execution over the validated byte-range payload page rows.
- Broader sustained worker scheduling beyond the current joined engine-owned page streaming worker proof.
- Native queue/fence/status integration and async overlap evidence.
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
