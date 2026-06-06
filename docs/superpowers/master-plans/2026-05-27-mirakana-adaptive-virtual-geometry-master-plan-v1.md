# Mirakana Adaptive Virtual Geometry Master Plan v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:writing-plans to create a focused dated child implementation plan before editing code. Child plans should then use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement task-by-task. Steps in child plans use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a clean-break, first-party virtualized clustered geometry system that can exceed Nanite-like static raster LOD by unifying static geometry, dynamic deformation, ray tracing, large-scene streaming, and temporal quality governance.

**Architecture:** Treat MAVG as a new geometry subsystem, not as a compatibility layer over conventional mesh LOD. Offline tools build a validated cluster graph; runtime systems select, stream, and render resident clusters through backend-neutral renderer/RHI contracts with D3D12, Vulkan, and host-gated Metal evidence. Every broad claim is fail-closed until backed by tests, package evidence, backend-local validation, profiling counters, docs, manifest rows, and legal/dependency review.

**Tech Stack:** C++23, `MK_assets`, `MK_tools`, `MK_renderer`, `MK_rhi`, `MK_runtime`, `MK_runtime_rhi`, `MK_runtime_scene_rhi`, `MK_scene_renderer`, CMake/CTest, PowerShell validation tools, D3D12 mesh shaders and indirect draws, Vulkan `VK_EXT_mesh_shader` and indirect draws, optional D3D12 Work Graphs research gates, Vulkan ray tracing acceleration-structure gates, GPU memory/residency policy, frame graph multi-queue execution, runtime package streaming, and clean-room public research inputs.

---

**Plan ID:** `mirakana-adaptive-virtual-geometry-master-plan-v1`

**Status:** Candidate long-range master plan. Not selected as `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan`.

**Date:** 2026-05-27

**Latest audit:** 2026-06-06. This audit keeps the plan as a candidate master roadmap and updates the clean-break baseline after first-party desktop platform, first-party UI/editor, environment, performance-foundation work, MAVG runtime LOD closeout, autonomous page streaming worker proof, deterministic automatic eviction candidate ordering, caller-supplied recency ordering, runtime-inferred selected resident page use-generation evidence, runtime-inferred LRU/frequency candidate ordering, value-only caller-supplied GPU memory pressure eviction planning, D3D12 DXGI diagnostics-to-pressure-row evidence, D3D12 committed-resource residency action execution, and optional DirectStorage file-to-memory native queue/status execution landed.

## Master Plan Decision

This should be a master plan, not a single active dated implementation plan.

Reasons:

- The work crosses asset import/cook, renderer, RHI, runtime streaming, scene rendering, profiling, validation, package evidence, legal review, and agent-surface contracts.
- It has multiple independently reviewable milestones that need their own API and validation boundaries.
- It should not displace the manifest-selected active plan or production-completion selection gate until the operator intentionally selects the first MAVG child plan.
- The "Nanite-exceeding" claim is a final integrated benchmark claim, not a useful early implementation target.

Use this file as the roadmap and decision contract. Create focused child plans under `docs/superpowers/plans/YYYY-MM-DD-mavg-<phase>.md` when executing a phase.

## 2026-06-05 Full Project Audit Addendum

This addendum is the current execution baseline for any future MAVG child plan.

Repository state checked during this audit:

- `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` now returns to `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`, with `recommendedNextPlan.id = next-production-gap-selection` and `unsupportedProductionGaps = []`, after the stacked LoD milestone plus follow-up automatic eviction, caller-supplied recency child `mavg-resident-page-recency-eviction-order-v1`, MAVG Runtime-Inferred Page Use Generation v1 child `mavg-runtime-inferred-page-use-generation-v1`, MAVG Runtime-Inferred LRU Eviction Policy v1 child `mavg-runtime-inferred-lru-eviction-policy-v1`, MAVG Runtime-Inferred Frequency Eviction Policy v1 child `mavg-runtime-inferred-frequency-eviction-policy-v1`, MAVG GPU Memory Pressure Eviction Policy v1 child `mavg-gpu-memory-pressure-eviction-policy-v1`, MAVG DXGI GPU Memory Pressure Evidence v1 child `mavg-dxgi-gpu-memory-pressure-evidence-v1`, MAVG D3D12 Residency Action Execution v1 child `mavg-d3d12-residency-action-execution-v1`, MAVG DirectStorage Native Execution v1 child `mavg-directstorage-native-execution-v1` with optional `MK_runtime_host_win32_directstorage` file-to-memory native queue/status execution, MAVG DirectStorage Native Fence Signal v1 child `mavg-directstorage-native-fence-signal-v1` with optional private `IDStorageQueue::EnqueueSignal` evidence, MAVG DirectStorage D3D12 Buffer Destination v1 child `mavg-directstorage-d3d12-buffer-destination-v1` with optional private D3D12 resource/buffer-resource destination evidence, MAVG DirectStorage RHI Resource Destination v1 child `mavg-directstorage-rhi-resource-destination-v1` with optional caller-owned D3D12 RHI `BufferHandle` destination evidence, MAVG Page GPU Buffer Destination Plan v1 child `mavg-page-gpu-buffer-destination-plan-v1` with value-only request-to-`BufferHandle` destination range planning evidence, and MAVG Page GPU Resource Update Readiness v1 child `mavg-page-gpu-resource-update-readiness-v1` / draft PR #505 with completed resource-destination evidence to resident resource row readiness evidence.
- MAVG remains a long-range roadmap. The completed stacked LoD work has implemented graph/payload/selection/page-streaming/conventional-renderer evidence, selected D3D12 indirect/GPU-culling proof rows, a joined page streaming worker boundary, deterministic automatic eviction candidate ordering, caller-supplied recency ordering, and `infer_runtime_mavg_resident_page_use_generations` value-only selected resident page use-generation evidence with `inferred_resident_page_use_generation`, plus `RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::runtime_inferred_lru`, `inferred_lru_eviction_policy`, `runtime_inferred_lru_eviction_candidate_count`, runtime-inferred frequency ordering, MAVG Runtime-Inferred Frequency Eviction Policy v1 (`mavg-runtime-inferred-frequency-eviction-policy-v1`), `RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::runtime_inferred_frequency`, `RuntimeMavgPageStreamingFrequencyRow`, `RuntimeMavgResidentPageFrequencyDesc`, `RuntimeMavgResidentPageFrequencyResult`, `infer_runtime_mavg_resident_page_frequencies`, `inferred_frequency_eviction_policy`, `runtime_inferred_frequency_eviction_candidate_count`, `frequency_counter_overflow`, MAVG GPU Memory Pressure Eviction Policy v1 (`mavg-gpu-memory-pressure-eviction-policy-v1`), `RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::caller_supplied_gpu_memory_pressure`, `RuntimeMavgPageStreamingGpuMemoryPressureRow`, `planned_gpu_memory_pressure_eviction_policy`, `applied_caller_supplied_gpu_memory_pressure_policy`, `gpu_memory_pressure_eviction_candidate_count`, `gpu_memory_pressure_counter_overflow`, MAVG DXGI GPU Memory Pressure Evidence v1 (`mavg-dxgi-gpu-memory-pressure-evidence-v1`), `mavg_gpu_memory_pressure.hpp`, `RuntimeMavgDxgiGpuMemoryPressureEvidenceDesc`, `RuntimeMavgResidentPageGpuMemoryEstimateRow`, `RuntimeMavgDxgiGpuMemoryPressureEvidenceResult`, `build_runtime_mavg_dxgi_gpu_memory_pressure_rows`, MAVG D3D12 Residency Action Execution v1 (`mavg-d3d12-residency-action-execution-v1`), `RhiResidencyActionDesc`, `RhiResidencyActionResult`, `IRhiDevice::execute_residency_action`, `ID3D12Device::MakeResident`, `ID3D12Device::Evict`, narrow evicted-resource direct-use/recording/submit rejection for active D3D12 rows, optional private DirectStorage D3D12 resource/buffer-resource destination proof through `DSTORAGE_REQUEST_DESTINATION_BUFFER`, and optional caller-owned D3D12 RHI buffer destination proof through `Win32MavgPayloadDirectStorageDispatcherDesc::directstorage_rhi_device`, `Win32MavgPayloadDirectStorageDispatcherDesc::directstorage_rhi_destination_buffer`, private `resolve_directstorage_buffer_destination`, and `used_directstorage_caller_owned_rhi_resource_destination`; full production streaming/residency services, full MAVG page-to-resource GPU residency service, allocator-integrated residency governance, allocator enforcement, GPU decompression, deformation, ray tracing, and benchmark superiority remain unimplemented and unclaimed.
- MAVG Runtime RHI Page Residency Actions v1 (`mavg-runtime-rhi-page-residency-actions-v1`) narrows the residency gap by adding `mavg_residency.hpp`, `RuntimeMavgResidentPageResourceRow`, `RuntimeMavgPageResidencyActionDesc`, `RuntimeMavgPageResidencyActionResult`, and `execute_runtime_mavg_page_residency_actions` so reviewed selected/protected resident pages and reviewed unprotected eviction candidates call `IRhiDevice::execute_residency_action`. MAVG DirectStorage RHI Resource Destination v1 narrows the IO destination gap by resolving caller-owned D3D12 `BufferHandle` rows for optional DirectStorage queue submissions. MAVG Page GPU Buffer Destination Plan v1 narrows the planning gap by adding `mavg_page_gpu_buffer_destination.hpp`, `RuntimeMavgPageBufferDestinationRow`, `RuntimeMavgPageBufferDestinationDesc`, `RuntimeMavgPageBufferDestinationPlanResult`, and `plan_runtime_mavg_page_buffer_destinations` so successful MAVG DirectStorage request rows map to caller-owned first-party RHI `BufferHandle` byte ranges before optional native execution. MAVG Page GPU Resource Update Readiness v1 / draft PR #505 narrows the handoff gap by adding `mavg_page_gpu_resource_update.hpp`, `RuntimeMavgPageGpuResourceUpdateReadinessDesc`, `RuntimeMavgPageGpuResourceUpdateReadinessResult`, `RuntimeMavgPageGpuResourceUpdateRow`, and `make_runtime_mavg_page_gpu_resource_update_readiness` so completed caller-owned RHI DirectStorage resource-destination dispatch/status evidence can become deterministic `RuntimeMavgResidentPageResourceRow` values before any residency action execution. The proof remains intentionally narrow: real GPU residency enforcement beyond reviewed action rows, allocator/GPU budget enforcement, automatic RHI resource allocation, public/native renderer/RHI handles beyond reviewed first-party descriptor rows, DirectStorage native fence integration into RHI residency, broader D3D12 resource-destination forms such as texture/tiled DirectStorage IO, GPU decompression, command-list residency-set scheduling, Vulkan/Metal parity, async-overlap/performance, mesh shaders, deformation, ray tracing, Nanite compatibility/equivalence/superiority, and benchmark superiority remain future work.
- MAVG Page GPU Resource Residency Execution v1 (`mavg-page-gpu-resource-residency-execution-v1`) adds the next narrow handoff by connecting successful `RuntimeMavgPageGpuResourceUpdateReadinessResult` rows to `execute_runtime_mavg_page_residency_actions` through `mavg_page_gpu_resource_residency_execution.hpp`, `RuntimeMavgPageGpuResourceResidencyExecutionDesc`, `RuntimeMavgPageGpuResourceResidencyExecutionResult`, and `execute_runtime_mavg_page_gpu_resource_residency_actions`. It preserves DirectStorage resource-destination completion evidence as data and proves selected/protected make-resident rows, protected eviction skips, reviewed unprotected eviction rows, missing/invalid/not-ready readiness rejection, delegated residency validation failure reporting, and non-claims for helper-owned queue submission, file IO, RHI allocation, allocator/GPU budget enforcement, mount mutation, GPU decompression, native handles, Vulkan/Metal native IO parity, async-overlap/performance, Nanite compatibility/equivalence/superiority, and benchmark superiority.
- MAVG GPU Cluster Format Rows v1 (`mavg-gpu-cluster-format-rows-v1`) adds the backend-neutral `MK_renderer` GPU layout preflight through `mavg_gpu_cluster_format.hpp`, `MavgGpuClusterFormatDesc`, `MavgGpuClusterFormatRow`, `MavgGpuClusterFormatPlan`, and `plan_mavg_gpu_cluster_format_rows`. It derives deterministic per-cluster graph/page/local ids, LOD/material/fallback rows, draw ranges, payload page byte ranges, 32-byte vertex / uint32 index layout evidence, bounds center/radius, and D3D12/Vulkan meshlet limit readiness counters from validated `MavgClusterGraphDocument` plus `MavgClusterPayloadDocument` rows. It narrows the GPU cluster format gap before mesh shader/GPU traversal work, while RHI allocation, GPU work submission, mesh shader execution, GPU traversal, GPU decompression, Vulkan/Metal parity, Nanite compatibility/equivalence/superiority, and benchmark superiority remain future work.
- MAVG GPU Visible Cluster Packet Rows v1 (`mavg-gpu-visible-cluster-packets-v1`) / draft PR #509 adds the backend-neutral `MK_renderer` logical packet preflight through `mavg_gpu_visible_cluster_packets.hpp`, `MavgGpuVisibleClusterPacketDesc`, `MavgGpuVisibleClusterPacketRow`, `MavgGpuVisibleClusterPacketPlan`, and `plan_mavg_gpu_visible_cluster_packets`. It joins successful `MavgGpuClusterFormatPlan` rows with successful `MavgGpuCullingIndirectPlan` visible command rows, preserves visible command order, carries payload byte spans, draw arguments, bounds, meshlet readiness, and fallback substitution provenance, and fails closed for invalid source plans, duplicate/missing format rows, stale visible commands, visible command count mismatches, meshlet readiness requirements, and packet budgets. It narrows the GPU traversal/visibility-buffer handoff gap before any actual GPU visibility-buffer allocation/write or traversal execution; RHI allocation, GPU work submission, mesh shader execution, indirect draw execution, GPU decompression, Vulkan/Metal parity, Nanite compatibility/equivalence/superiority, and benchmark superiority remain future work.
- MAVG GPU Visibility Buffer Layout v1 (`mavg-gpu-visibility-buffer-layout-v1`) adds the backend-neutral `MK_renderer` logical visibility-buffer layout preflight through `mavg_gpu_visibility_buffer_layout.hpp`, `MavgGpuVisibilityBufferLayoutDesc`, `MavgGpuVisibilityBufferSlotRow`, `MavgGpuVisibilityBufferByteRangeRow`, `MavgGpuVisibilityBufferLayoutRow`, `MavgGpuVisibilityBufferSyncIntentRow`, `MavgGpuVisibilityBufferLayoutPlan`, and `plan_mavg_gpu_visibility_buffer_layout`. It consumes successful visible packet rows and emits deterministic slot, byte-range, record-layout, and backend-neutral write-to-read sync-intent rows while failing closed for missing/invalid packet plans, count mismatches, duplicate/non-dense packet indexes, meshlet readiness requirements, slot stride/alignment, slot budgets, and byte-range overflow. It narrows the GPU visibility-buffer handoff gap before actual GPU visibility-buffer allocation/write or traversal execution; RHI allocation, GPU work submission, mesh shader execution, indirect draw execution, GPU decompression, Vulkan/Metal parity, Nanite compatibility/equivalence/superiority, and benchmark superiority remain future work.
- SDL3 is not an active dependency or supported runtime/editor/audio path. The completed first-party desktop platform milestone replaced SDL3 surfaces with `MK_platform_win32`, `MK_runtime_host_win32`, `MK_runtime_host_win32_presentation`, and `MK_audio_wasapi`.
- The active visible editor shell is first-party retained UI over private Win32, Direct3D 12, DirectWrite, TSF, and UIA adapters. Dear ImGui, SDL3, Qt, Slint, RmlUi, and UI middleware must not be reintroduced for MAVG tools or benchmark UI.
- `MK_environment` is now a renderer and scene-renderer dependency. MAVG benchmark scenes must account for selected sky, fog, cloud, rain, time-of-day, and weather-blending evidence without implying broad `environment_ready`, snow package readiness, volumetric-cloud package readiness, Vulkan/Metal parity, or broad environment optimization.
- Performance foundation work has landed for performance budgets/evidence, memory diagnostics, frame/thread scratch, job scheduling, worker pools, work stealing, topology and placement policy, Windows CPU Sets worker placement, SIMD dispatch, AVX2 reviewed target execution, CPU profiling matrix, optional GPU compute review, and long-running readiness gates. MAVG must build on these rows before making CPU/GPU/memory optimization claims.
- Current `vcpkg.json` has no default dependencies. Optional features remain explicit and reviewed; MAVG must not add dependencies without `vcpkg.json`, `docs/dependencies.md`, `docs/legal-and-licensing.md`, and `THIRD_PARTY_NOTICES.md` updates in the same child plan.

Audit conclusions:

- Keep this file as the master plan. Do not create a second MAVG master plan unless this one is formally superseded.
- First executable work should be a dated Phase 0 child plan that refreshes official-source checks, cleans stale planning assumptions, and locks benchmark methodology before code.
- MAVG code and tools must stay clean-break: no backward-compatibility aliases, no old SDL3/Dear ImGui lane, no public native backend handles, and no UE/Nanite compatibility.
- The near-term claim should be "static clustered geometry with deterministic fallback and package evidence." "Exceeds Nanite-like LOD" remains a measured final claim across selected axes only.

Audit-detected documentation drift to reconcile before implementation:

- Historical docs may still mention earlier Dear ImGui/desktop-gui guidance. Child plans must treat `docs/current-capabilities.md`, `docs/superpowers/plans/README.md`, `engine/agent/manifest.json`, and `docs/agent-operational-reference.md` as current truth, then update stale durable docs if the child plan relies on them.
- Any child plan that touches editor shell, native desktop, renderer, package, validation, or agent contracts must run an agent-surface drift check and update the owning docs/manifest/static checks rather than copying historical claims forward.

## Clean-Room And Legal Guardrails

- Do not read, copy, port, transcribe, or pattern-match Unreal Engine source code, shaders, internal tools, cooked data, or private presentations.
- Do not use `Nanite` as a product/API/asset/validation name. Use `Mirakana Adaptive Virtual Geometry` or `MAVG`.
- Use only public documentation, public papers, official SDK specifications, and first-party original code.
- Before commercial distribution or marketing claims, run a freedom-to-operate review with counsel or a patent attorney. This plan is technical planning, not legal advice.
- Do not import third-party simplification, meshlet, compression, ray tracing, or streaming code unless the license is explicit and the repository dependency/legal records are updated in the same child plan.

## Official And Primary Sources

Use current official documentation before implementing each child phase. Re-check these links during execution because SDK details can change:

- Epic Nanite overview and technical details: `https://dev.epicgames.com/documentation/en-us/unreal-engine/nanite-virtualized-geometry-in-unreal-engine`, `https://dev.epicgames.com/documentation/unreal-engine/nanite-technical-details?application_version=5.7`
- Microsoft DirectX Mesh Shader spec: `https://microsoft.github.io/DirectX-Specs/d3d/MeshShader.html`
- Microsoft DirectX Work Graphs spec: `https://microsoft.github.io/DirectX-Specs/d3d/WorkGraphs.html`
- Khronos Vulkan mesh shader extension: `https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_mesh_shader.html`
- Khronos Vulkan synchronization and validation guidance: `https://docs.vulkan.org/guide/latest/synchronization.html`, `https://docs.vulkan.org/guide/latest/validation_overview.html`
- NVIDIA RTX Mega Geometry public guidance: `https://developer.nvidia.com/blog/nvidia-rtx-mega-geometry-now-available-with-new-vulkan-samples/`
- AMD GPUOpen meshlet compression guidance: `https://gpuopen.com/learn/mesh_shaders/mesh_shaders-meshlet_compression/`
- Garland and Heckbert, quadric error metrics: `https://publications.ri.cmu.edu/surface-simplification-using-quadric-error-metrics`
- Hoppe, progressive meshes: `https://hhoppe.com/proj/pm/`
- CMake presets, install/export, package config, and CXX module file-set guidance: `https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html`, `https://cmake.org/cmake/help/latest/command/install.html`, `https://cmake.org/cmake/help/latest/command/export.html`, `https://cmake.org/cmake/help/latest/manual/cmake-cxxmodules.7.html`
- Microsoft Win32 Raw Input: `https://learn.microsoft.com/en-us/windows/win32/inputdev/about-raw-input`
- Microsoft WASAPI/Core Audio: `https://learn.microsoft.com/en-us/windows/win32/coreaudio/wasapi`
- Microsoft DirectWrite: `https://learn.microsoft.com/en-us/windows/win32/directwrite/direct-write-portal`
- Microsoft Text Services Framework: `https://learn.microsoft.com/en-us/windows/win32/tsf/text-services-framework`
- Microsoft UI Automation provider/client specifications: `https://learn.microsoft.com/en-us/windows/win32/winauto/ui-automation-specification`
- Apple Metal feature set tables and mesh/object shader documentation: `https://developer.apple.com/metal/capabilities/`, `https://developer.apple.com/documentation/metal/mesh-and-object-shader-resource-preparation-commands`

Use official SDK docs as constraints, not as copied implementation. Use the research papers for algorithmic ideas and write original implementations.

Source-use rules from the 2026-06-05 audit:

- Use Context7 for live CMake and Vulkan documentation checks during child-plan authoring.
- Use official Microsoft, Khronos, Apple, Epic, NVIDIA, and AMD documentation for API constraints and capability gates.
- Treat vendor blog guidance as design input only; do not add vendor-only APIs to the default path.
- Treat Epic Nanite documentation as a feature taxonomy and support-boundary reference only. Do not use UE source, shaders, internal tools, cooked data, or private presentations.

## Current Engine Anchors

Existing usable foundations:

- `MK_platform_win32`, `MK_runtime_host_win32`, `MK_runtime_host_win32_presentation`, and `MK_audio_wasapi` are the active Windows desktop/runtime/audio host foundations. MAVG sample and benchmark UI must target these first-party host lanes, not SDL3.
- `MK_editor_core`, `mirakana::ui`, `MK_ui`, and `MK_ui_renderer` are the active first-party UI/editor foundations. MAVG editor-facing diagnostics must use retained first-party rows and private native adapters.
- `MK_environment` provides profile validation/text IO/package rows, scene/runtime environment profile binding, renderer policy planning, selected D3D12 sky/fog/cloud/rain evidence, and host-gated Vulkan height-fog proof. MAVG benchmark content should include environment load but avoid broad environment-ready claims.
- Performance foundation rows exist for performance budget evidence, memory diagnostics, scratch arenas, job execution, CPU placement, SIMD/AVX2 dispatch, long-running readiness, CPU profiling matrix, and optional GPU compute review. MAVG readiness claims must cite or extend these rows rather than inventing parallel diagnostics.
- MAVG runtime streaming foundations now include reviewed page request planning, selected visible/fallback ancestor protection, safe-point/background dispatch planning, a joined engine-owned page streaming worker boundary, deterministic resident-page automatic eviction candidate ordering through `plan_runtime_mavg_page_streaming_automatic_evictions`, caller-supplied recency ordering through `RuntimeMavgPageStreamingRecencyRow`, `RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::caller_supplied_recency`, and `resident_page_last_used_generation`, selected resident page use-generation inference through `RuntimeMavgResidentPageUseGenerationDesc`, `RuntimeMavgResidentPageUseGenerationResult`, and `infer_runtime_mavg_resident_page_use_generations`, explicit runtime-inferred LRU/frequency ordering through `runtime_inferred_lru` / `runtime_inferred_frequency`, value-only caller-supplied GPU memory pressure ordering through `RuntimeMavgPageStreamingGpuMemoryPressureRow` and `RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::caller_supplied_gpu_memory_pressure`, D3D12 DXGI diagnostics-to-pressure-row generation through `build_runtime_mavg_dxgi_gpu_memory_pressure_rows`, and a narrow D3D12 committed-resource residency action boundary through `RhiResidencyActionDesc`, `RhiResidencyActionResult`, and `IRhiDevice::execute_residency_action`.
- `engine/runtime/include/mirakana/runtime/entity_scale_culling.hpp` has value-only LOD band, visibility, draw/update cost, and budget planning.
- `engine/renderer/include/mirakana/renderer/scene_scale_policy.hpp` has backend-neutral scene scale, culling, batching, and LOD policy rows.
- `engine/runtime_rhi/include/mirakana/runtime_rhi/runtime_upload.hpp` has runtime mesh, skinned mesh, morph mesh, and texture upload evidence.
- `engine/renderer/include/mirakana/renderer/frame_graph_rhi.hpp` has multi-queue frame graph command submission, render-pass envelopes, barriers, transient texture lifetimes, and package evidence hooks.
- `engine/renderer/include/mirakana/renderer/gpu_memory_policy.hpp` has memory/residency policy evidence rows.
- `engine/rhi/include/mirakana/rhi/rhi.hpp` has backend-neutral stats, queues, buffers, textures, draw counters, and transient resource counters.
- D3D12 and Vulkan backends already carry selected package/evidence lanes; Metal remains Apple-host-gated.

Important gaps:

- No production compressed meshlet binary / shader-consumable GPU payload format beyond the current validated `MavgClusterGraphDocument`, `MavgClusterPayloadDocument`, and value-only MAVG GPU Cluster Format Rows v1 metadata.
- No cluster hierarchy builder.
- No cluster simplification/error metric.
- No GPU cluster traversal or allocated/written GPU visibility buffer beyond current value-only visible cluster packet and visibility-buffer layout rows.
- No mesh shader or indirect clustered geometry path.
- No full production cluster page streaming/residency service beyond the current reviewed request planning, joined worker boundary, deterministic resident-page automatic eviction candidate ordering, caller-supplied recency ordering, runtime-inferred LRU/frequency ordering, value-only caller-supplied GPU memory pressure ordering, D3D12 diagnostics-to-pressure-row adapter, narrow D3D12 committed-resource `MakeResident` / `Evict` action plus evicted-resource use rejection boundary, optional private DirectStorage D3D12 resource/buffer-resource destination proof, and optional caller-owned D3D12 RHI buffer destination proof; full MAVG page-to-resource GPU residency service, allocator-integrated residency governance, allocator enforcement, backend-native pressure harvesting beyond `RhiDeviceMemoryDiagnostics`, and GPU decompression remain future work.
- No unified raster/ray tracing cluster payload.
- No deformation-safe cluster bounds or dynamic update policy.
- No benchmark harness capable of supporting a "beyond Nanite-like LOD" claim.
- No MAVG-specific first-party editor/benchmark panel, diagnostic row family, package recipe, manifest row, or validation recipe.

## Success Criteria

MAVG can be called production-ready only when all of these are true:

- Static, skinned, morph, and selected deformable geometry use the same first-party cluster graph contract.
- Runtime never drops required geometry solely because a fine cluster is missing; resident ancestor fallback is deterministic.
- GPU selection respects frustum, occlusion, screen-space error, memory budget, backend capability, and temporal stability.
- Raster and ray tracing payloads derive from the same cooked cluster graph, with explicit consistency diagnostics.
- D3D12 and Vulkan have backend-local validation. Metal remains host-gated until Apple-host evidence lands.
- The system reports quality, streaming, residency, culling, draw, shader, ray tracing, and degradation counters through public first-party diagnostics.
- Benchmarks compare MAVG against the existing conventional mesh path and against documented Nanite-like feature classes without using UE source or data.
- Docs, manifests, schemas, package recipes, static checks, skills, and validation scripts match the implemented scope.

## Non-Goals

- No Unreal Engine compatibility, Nanite asset compatibility, or UE cooked-data import.
- No compatibility shims for old experimental MAVG APIs once a phase lands.
- No public native backend handles in gameplay APIs.
- No dependency on vendor-only APIs for the default path.
- No broad Metal readiness until validated on an Apple host.
- No marketing claim that MAVG "beats Nanite" until benchmark methodology and measured results exist.

## Accuracy Of Estimate

This is the most defensible estimate with current information. It assumes senior engineers familiar with modern rendering, C++23, D3D12/Vulkan, asset cooking, and GPU profiling.

The 2026-06-05 audit improves confidence for platform/UI/performance foundations but does not shorten the hardest MAVG work: original hierarchy generation, streaming, backend execution, RT consistency, deformation quality, and benchmark proof still dominate the schedule.

| Scope | Solo senior engineer | 3 senior engineers | Confidence |
| --- | ---: | ---: | --- |
| Static cluster asset graph plus CPU selection prototype | 3-5 months | 2-3 months | Medium |
| Static GPU clustered renderer with D3D12 and Vulkan evidence | 7-12 months | 4-7 months | Medium-low |
| Streaming, graceful degradation, profiling, environment-aware package proof, and benchmark scenes | 12-20 months | 7-11 months | Medium-low |
| Deformable clusters plus raster/RT consistency integration | 22-34 months | 12-20 months | Low |
| Integrated "exceeds Nanite-like LOD on selected axes" benchmark claim | 24-42 months | 14-24 months | Low |

Minimum useful production-facing v1: 6-10 person-months.

Credible flagship system across all requested axes: 42-72 person-months, plus host validation and legal/FTO review.

Main uncertainty drivers:

- Quality of original cluster hierarchy and simplification implementation.
- Backend mesh shader maturity and fallback performance.
- Streaming and residency behavior under real open-world content.
- Deformation bounds tightness versus runtime cost.
- Ray tracing acceleration-structure update cost.
- Availability of hardware/host matrix for D3D12, Vulkan, and Metal.

## Architecture Overview

### Offline Cook

The cook pipeline converts source meshes into a `MAVGClusterGraph`.

Core rows:

- `MavgSourceMeshRow`: source identity, material partitions, skin/morph support, and import provenance.
- `MavgClusterRow`: compressed vertex/index range, local bounds, normal cone, material id, primitive count, vertex count, and parent/child rows.
- `MavgClusterErrorRow`: geometric error, silhouette error, normal/material error, deformation expansion, and ray tracing error.
- `MavgClusterPageRow`: page id, compressed byte range, dependency rows, resident ancestor, and streaming priority metadata.
- `MavgClusterRtRow`: BLAS or cluster-AS build/refit policy, geometry flags, opacity/mask metadata, and fallback rows.

Rules:

- Split by material, topology constraints, skin influence policy, morph stream policy, and cluster size.
- Build hierarchy through original simplification code using QEM-style geometric error, but extend it with silhouette and material/error terms.
- Guarantee a resident fallback ancestor for every streamable cluster.
- Validate crack boundaries and reject a graph if sibling/parent replacement can create visible holes.
- Produce deterministic binary and JSON metadata suitable for package evidence.

### Runtime Selection

Runtime selection chooses resident clusters for each view.

Inputs:

- Camera/projection, view frustum, frame time budget, memory budget, backend capability, view history, motion vectors, and ray tracing requirement rows.

Outputs:

- Selected cluster ids.
- Missing cluster requests.
- Resident fallback substitutions.
- Draw, mesh shader, indirect, or fallback path rows.
- Quality degradation rows with explicit reason codes.

Rules:

- Missing fine clusters fall back to resident ancestors, not holes.
- Over-budget frames increase allowable error using a stable budget solver.
- Temporal hysteresis prevents rapid LOD oscillation.
- Ray tracing selection must either use matching cluster payloads or report a consistency diagnostic.

### GPU Execution

GPU execution should progress in layers:

- Compute culling plus indirect draw fallback.
- D3D12 mesh shader path.
- Vulkan `VK_EXT_mesh_shader` path.
- Optional D3D12 Work Graphs experiment after the baseline path is proven.
- Metal path only under Apple-host validation.

Do not make mesh shaders mandatory for package consumption. The default production path must have a compute/indirect fallback until backend coverage is proven.

GPU synchronization rules:

- D3D12 and Vulkan paths must record explicit transitions/barriers from compute shader writes to indirect command reads and draw/mesh shader reads.
- Vulkan child plans must use synchronization2-era reasoning and strict validation-layer evidence where the host/toolchain supports it.
- D3D12 mesh shader paths must query mesh shader and mesh shader pipeline statistics support before relying on those counters.
- Work Graphs are research-gated until the conventional compute/indirect and mesh shader paths have measured evidence.

### Streaming And Residency

Cluster pages are explicit runtime resources.

Rules:

- Residency is selected by MAVG policy rows, not ad hoc renderer ownership.
- Page requests are priority sorted by screen error, fallback quality loss, camera velocity, and RT need.
- Eviction cannot remove the last resident fallback ancestor of any visible cluster.
- A page may be unavailable without causing missing geometry.
- GPU memory policy gates broad streaming claims.

### Deformation

Dynamic geometry is first-class, but not free.

Deformation tiers:

- Tier 0: static clusters.
- Tier 1: rigid instances.
- Tier 2: skinned clusters with conservative per-cluster bone bounds.
- Tier 3: morph clusters with precomputed delta bounds.
- Tier 4: runtime displacement or destruction with dynamic recluster/update budget.

Each tier must have its own diagnostics. Unsupported tiers fall back to conventional rendering until the tier lands; they do not pretend to be MAVG-ready.

### Ray Tracing Integration

Ray tracing is integrated through explicit payload rows.

Rules:

- Raster and RT payloads derive from the same cluster graph.
- BLAS build/refit policy is chosen per cluster page and deformation tier.
- RT fallback must be quality-diagnosed; it cannot silently use unrelated coarse meshes.
- Vulkan/D3D12 RT feature availability and build/update costs are backend-local evidence.

## Milestone Plan

### Phase 0 - Research, Legal, And Benchmark Baseline

**Estimate:** 3-5 weeks solo, 2-3 weeks with 3 engineers.

**Files:**

- Create if Phase 0 starts on the current audit date: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Create if Phase 0 starts on the current audit date: `docs/specs/2026-06-05-mavg-benchmark-methodology-v1.md`
- If Phase 0 starts later, use the actual child-plan authoring date per `docs/superpowers/plans/README.md`.
- Modify: `docs/dependencies.md`
- Modify: `docs/legal-and-licensing.md`
- Modify: `THIRD_PARTY_NOTICES.md` only if a dependency or dataset is selected.
- Modify stale durable guidance only when the Phase 0 spec relies on it, including `docs/architecture-directory-verification.md`, `docs/cpp-standard.md`, `docs/current-capabilities.md`, and `docs/roadmap.md`.

**Deliverables:**

- Official-doc check matrix for D3D12, Vulkan, Metal, ray tracing, CMake/CXX modules, Win32 input/audio/text/accessibility, and first-party UI/editor constraints.
- Clean-room record: what public sources are allowed, what is forbidden, and who reviewed it.
- Benchmark scenes selected from first-party/generated assets only.
- Baseline counters for conventional static mesh, skinned mesh, morph mesh, package streaming, memory, frame graph, renderer quality, environment rendering, job execution, and long-run readiness.
- Updated stale-doc inventory so historical Dear ImGui/desktop-gui/SDL3 references are classified as historical, corrected, or removed from active guidance.
- First child plan selection.

**Validation:**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
git diff --check
```

### Phase 1 - MAVG Asset Graph v1

**Estimate:** 8-12 weeks solo, 4-6 weeks with 3 engineers.

**Files:**

- Create: `engine/assets/include/mirakana/assets/mavg_cluster_graph.hpp`
- Create: `engine/assets/src/mavg_cluster_graph.cpp`
- Create: `engine/tools/include/mirakana/tools/mavg_cluster_cook.hpp`
- Create: `engine/tools/asset/mavg_cluster_cook.cpp`
- Modify: `engine/tools/CMakeLists.txt`
- Create: `tests/unit/mavg_cluster_graph_tests.cpp`
- Create: `tests/unit/tools_mavg_cluster_cook_tests.cpp`
- Modify: `CMakeLists.txt`

**Deliverables:**

- Deterministic cluster graph structs and validators.
- Simple first-party cluster splitter for static mesh payloads.
- Parent/child hierarchy metadata.
- Resident fallback ancestor validation.
- Package metadata text format with stable hashes.
- RED/GREEN tests for invalid topology, material splits, missing fallback, duplicate cluster ids, and non-deterministic output.

**Done when:**

- A source static mesh can produce a valid cluster graph package with deterministic bytes and no renderer dependency.

### Phase 2 - MAVG CPU Selection v1

**Estimate:** 6-9 weeks solo, 3-5 weeks with 3 engineers.

**Files:**

- Create: `engine/renderer/include/mirakana/renderer/mavg_selection.hpp`
- Create: `engine/renderer/src/mavg_selection.cpp`
- Create: `tests/unit/mavg_selection_tests.cpp`
- Modify: `engine/renderer/CMakeLists.txt`

**Deliverables:**

- CPU reference selector for frustum, screen-space error, resident fallback, budget degradation, and temporal hysteresis.
- Value-only public rows for selected clusters, missing pages, fallback substitutions, and quality diagnostics.
- Exact tests for stable selection, no holes, over-budget degradation, and temporal stability.

**Done when:**

- CPU selection produces deterministic cluster rows and never reports missing visible geometry when a valid fallback ancestor exists.

### Phase 3 - Conventional Render Adoption v1

**Estimate:** 7-11 weeks solo, 4-6 weeks with 3 engineers.

**Files:**

- Create: `engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_upload.hpp`
- Create: `engine/runtime_rhi/src/mavg_upload.cpp`
- Create: `engine/scene_renderer/include/mirakana/scene_renderer/mavg_scene_renderer.hpp`
- Create: `engine/scene_renderer/src/mavg_scene_renderer.cpp`
- Create: `tests/unit/runtime_rhi_mavg_upload_tests.cpp`
- Create: `tests/unit/scene_renderer_mavg_tests.cpp`
- Modify: `engine/runtime_rhi/CMakeLists.txt`
- Modify: `engine/scene_renderer/CMakeLists.txt`

**Deliverables:**

- Upload selected cluster payloads through existing runtime upload/frame graph paths.
- Submit selected clusters through the existing draw/indexed draw path.
- Report MAVG draw counters without mesh shader dependency.
- Package sample smoke for a selected first-party static clustered mesh.
- Include selected environment scene load in benchmark/package evidence without promoting broad `environment_ready`.

**Done when:**

- MAVG static geometry renders through the conventional backend path with package-visible counters and no public native handles.

### Phase 4 - GPU Culling And Indirect Draw v1

**Estimate:** 9-15 weeks solo, 5-8 weeks with 3 engineers.

**Files:**

- Create: `engine/rhi/include/mirakana/rhi/indirect_draw.hpp`
- Modify: `engine/rhi/include/mirakana/rhi/rhi.hpp`
- Modify: `engine/rhi/src/null_rhi.cpp`
- Modify: `engine/rhi/d3d12/src/d3d12_backend.cpp`
- Modify: `engine/rhi/vulkan/src/vulkan_backend.cpp`
- Create: `engine/renderer/include/mirakana/renderer/mavg_gpu_culling.hpp`
- Create: `engine/renderer/src/mavg_gpu_culling.cpp`
- Create: `tests/unit/mavg_gpu_culling_tests.cpp`
- Modify or create backend tests for D3D12/Vulkan indirect command evidence.

**Deliverables:**

- Backend-neutral indirect draw command buffers.
- Compute culling output buffers.
- Synchronization rows for compute-write to indirect-read and draw-read.
- D3D12 and Vulkan backend-local evidence.
- Null RHI deterministic counters.
- Vulkan synchronization2 and D3D12 barrier evidence for compute/indirect/draw resource transitions.

**Done when:**

- GPU culling selects visible clusters and submits indirect draw work with synchronization counters validated on D3D12 and Vulkan.

### Phase 5 - Cluster Streaming And Residency v1

**Estimate:** 10-14 weeks solo, 6-8 weeks with 3 engineers.

**Files:**

- Create: `engine/runtime/include/mirakana/runtime/mavg_streaming.hpp`
- Create: `engine/runtime/src/mavg_streaming.cpp`
- Create: `engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_residency.hpp`
- Create: `engine/runtime_rhi/src/mavg_residency.cpp`
- Create: `tests/unit/mavg_streaming_tests.cpp`
- Create: `tests/unit/runtime_rhi_mavg_residency_tests.cpp`

**Deliverables:**

- Cluster page request queue.
- Resident page catalog.
- Eviction policy that preserves fallback ancestors.
- GPU memory policy integration.
- Package streaming evidence with missing-fine-cluster fallback.

**Done when:**

- Large MAVG packages can stream cluster pages without holes and with deterministic degradation rows under memory pressure.

### Phase 6 - Mesh Shader Backend v1

**Estimate:** 9-15 weeks solo, 5-9 weeks with 3 engineers.

**Files:**

- Create: `shaders/mavg_cluster_mesh.hlsl`
- Create or modify Vulkan SPIR-V shader artifacts and validation hooks.
- Modify: `engine/rhi/d3d12/src/d3d12_backend.cpp`
- Modify: `engine/rhi/vulkan/src/vulkan_backend.cpp`
- Create: `engine/renderer/include/mirakana/renderer/mavg_mesh_shader_policy.hpp`
- Create: `engine/renderer/src/mavg_mesh_shader_policy.cpp`
- Create: `tests/unit/mavg_mesh_shader_policy_tests.cpp`

**Deliverables:**

- D3D12 mesh shader execution path.
- Vulkan `VK_EXT_mesh_shader` path when toolchain and device support exist.
- Compute/indirect fallback remains the default when mesh shaders are unavailable.
- Shader validation and package counters are exact per backend.
- Mesh shader output limits, payload limits, pipeline statistics feature queries, and fallback diagnostics are represented as first-party rows.

**Done when:**

- MAVG mesh shader execution is proven on selected D3D12 and Vulkan hosts without weakening the fallback path.

### Phase 7 - Deformable Cluster Geometry v1

**Estimate:** 14-24 weeks solo, 8-14 weeks with 3 engineers.

**Files:**

- Create: `engine/assets/include/mirakana/assets/mavg_deformation.hpp`
- Create: `engine/assets/src/mavg_deformation.cpp`
- Create: `engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_deformation_upload.hpp`
- Create: `engine/runtime_rhi/src/mavg_deformation_upload.cpp`
- Modify: `engine/animation/` only for first-party data contract integration if required.
- Create: `tests/unit/mavg_deformation_tests.cpp`

**Deliverables:**

- Skinned cluster conservative bounds.
- Morph delta cluster bounds.
- Deformation tier diagnostics.
- Runtime update/refit policy rows.
- Package evidence for selected skinned and morph cluster samples.

**Done when:**

- Static, skinned, and morph geometry share the MAVG graph contract with explicit deformation quality and bounds diagnostics.

### Phase 8 - Ray Tracing Integration v1

**Estimate:** 14-22 weeks solo, 8-14 weeks with 3 engineers.

**Files:**

- Create: `engine/renderer/include/mirakana/renderer/mavg_ray_tracing.hpp`
- Create: `engine/renderer/src/mavg_ray_tracing.cpp`
- Modify D3D12/Vulkan backend ray tracing surfaces only after a focused backend design plan exists.
- Create: `tests/unit/mavg_ray_tracing_tests.cpp`

**Deliverables:**

- Raster/RT consistency rows derived from the same cluster graph.
- BLAS/refit/rebuild policy rows.
- Deformed cluster RT diagnostics.
- Backend feature gates.

**Done when:**

- MAVG can produce selected RT payload evidence without silently falling back to unrelated coarse meshes.

### Phase 9 - Quality Governor And Benchmarks v1

**Estimate:** 8-14 weeks solo, 5-8 weeks with 3 engineers.

**Files:**

- Create: `engine/renderer/include/mirakana/renderer/mavg_quality_governor.hpp`
- Create: `engine/renderer/src/mavg_quality_governor.cpp`
- Create: `tools/benchmark-mavg.ps1`
- Create: `docs/mavg.md`
- Create: `tests/unit/mavg_quality_governor_tests.cpp`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `engine/agent/manifest.fragments/*.json` only when selecting or closing implementation milestones.

**Deliverables:**

- Unified quality budget over raster, RT, streaming, deformation, and temporal stability.
- Benchmark harness with stable first-party scenes.
- Quality counters: screen error, silhouette error, material error, cluster fallback rate, page miss rate, temporal churn, RT consistency, CPU/GPU time, memory, and draw/dispatch counts.
- Explicit comparison against conventional renderer path.

**Done when:**

- A measured benchmark can state exactly which axes MAVG exceeds Nanite-like LOD classes on, without UE source/data and without broad unsupported claims.

## Validation Strategy

Each child plan must run the narrowest focused loop first and full validation at the coherent slice gate.

Baseline commands:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Additional phase gates:

- Docs/agent-only phases: `tools/check-text-format.ps1`, `tools/check-ai-integration.ps1`, `tools/check-agents.ps1`, `tools/check-json-contracts.ps1` if manifest fragments are touched, and `git diff --check`.
- Native desktop/editor phases: `tools/check-native-desktop-contracts.ps1` plus `tools/build-editor.ps1` when visible editor shell evidence is affected.
- Shader phases: `tools/check-shader-toolchain.ps1` plus DXIL/SPIR-V validation.
- Package phases: `tools/package-desktop-runtime.ps1` and installed runtime validation.
- Dependency phases: `tools/bootstrap-deps.ps1`, `tools/check-dependency-policy.ps1`, legal docs, and notices.
- Vulkan phases: strict validation-layer evidence and SPIR-V evidence.
- Metal phases: Apple-host evidence only.
- Benchmark phases: repeatable runs with raw output stored outside source or as intentionally tracked summarized evidence.
- Public API phases: `tools/check-public-api-boundaries.ps1` and targeted tests before full `tools/validate.ps1`.

## Agent-Surface Drift Policy

Every child plan must check whether its changes affect durable AI-operable contracts.

Likely affected surfaces:

- `docs/current-capabilities.md`
- `docs/roadmap.md`
- `docs/mavg.md`
- `docs/superpowers/plans/README.md`
- `engine/agent/manifest.fragments/*.json`
- `schemas/engine-agent.schema.json`
- `schemas/game-agent.schema.json`
- `.agents/skills/rendering-change/SKILL.md`
- `.agents/skills/performance-optimization-change/SKILL.md`
- `.claude/skills/gameengine-rendering/SKILL.md`
- `.claude/skills/gameengine-performance-optimization/SKILL.md`
- `.cursor/skills/gameengine-rendering/SKILL.md`
- `.cursor/skills/gameengine-performance-optimization/SKILL.md`
- `tools/check-ai-integration*.ps1`
- Package sample `game.agent.json` files.

Do not update all surfaces mechanically. Update only the surfaces whose durable behavior, validation, or AI-operable contract changed.

## Risk Register

| Risk | Severity | Mitigation |
| --- | --- | --- |
| Patent/FTO issue around virtualized geometry | High | Clean-room design, avoid UE source, counsel review before commercial claim. |
| Scope explosion | High | Master plan plus focused child plans; no single child plan spans more than one API/validation boundary. |
| Mesh shader availability gaps | High | Keep compute/indirect fallback as production path until backend evidence exists. |
| Reintroducing removed UI/platform middleware | High | MAVG tools and benchmarks use first-party UI/editor and Win32/WASAPI host lanes; SDL3, Dear ImGui, Qt, Slint, RmlUi, and UI middleware stay out unless a new architecture decision explicitly reverses policy. |
| Stale historical docs leaking into implementation | High | Phase 0 reconciles active truth against manifest/current-capabilities/plan registry and updates stale durable docs before code depends on them. |
| Streaming holes or popping | High | Resident fallback ancestor invariant and temporal quality governor are hard requirements. |
| Deformation bounds too conservative | Medium-high | Tiered deformation support with quality diagnostics and fallback to conventional rendering for unsupported tiers. |
| RT update cost too high | Medium-high | BLAS/refit/rebuild policy rows and selected benchmark gates before readiness claims. |
| Backend divergence | Medium-high | Per-backend proof rows; no proof inheritance between D3D12, Vulkan, and Metal. |
| Validation time | Medium | Phase-local tests first; full `tools/validate.ps1` at slice gate only. |

## Selection Rule

Do not start implementation from this master plan directly. Select the next child plan only when the operator is ready to pause or supersede the current active milestone.

Recommended first child:

- `docs/superpowers/plans/YYYY-MM-DD-mavg-research-legal-benchmark-baseline-v1.md`

Recommended first implementation child after that:

- `docs/superpowers/plans/2026-06-05-mavg-asset-graph-v1.md`

The first production-facing claim should be "static clustered geometry with deterministic fallback and package evidence", not "Nanite replacement".
