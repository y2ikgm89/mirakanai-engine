# 2026-06-21 MAVG Advanced Backend Evidence v1

## Purpose

Define the fail-closed claim matrix for the remaining MAVG advanced backend evidence cluster. This specification converts official-source requirements into deterministic package-visible row ids before any mesh shader, Metal, ray tracing, deformation, autonomous scheduling, broad optimization, or Nanite comparison work can be promoted.

This is not a backend execution proof and it is not a broad readiness claim.

## Status

Task 1 contract for `mavg-advanced-backend-evidence-closeout-v1`.

Implemented surface:

- `engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_advanced_backend_evidence.hpp`
- `MavgAdvancedBackendEvidenceDesc`
- `MavgAdvancedBackendEvidenceResult`
- `evaluate_mavg_advanced_backend_evidence`
- `MK_runtime_rhi_mavg_advanced_backend_evidence_tests`

Follow-on Task 5 surface:

- `mavg-autonomous-streaming-scheduler-v1`
- `engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_autonomous_streaming_scheduler.hpp`
- `RuntimeMavgAutonomousStreamingSchedulerState`
- `RuntimeMavgAutonomousStreamingSchedulerDesc`
- `RuntimeMavgAutonomousStreamingSchedulerResult`
- `tick_runtime_mavg_autonomous_streaming_scheduler`
- `MK_runtime_rhi_mavg_autonomous_streaming_scheduler_tests`
- `mavg_autonomous_streaming_scheduler_ready=1`

The Task 5 ready row is limited to caller-owned runtime tick orchestration over existing LOD/page-streaming/payload-IO/background-service/GPU-memory-residency/safe-point-adoption helpers. It does not expose native handles, execute renderer/RHI backends, prove async-overlap/performance proof, infer Metal readiness, compare against Nanite, or promote broad MAVG backend readiness.

## Non-Overlap

Completed MAVG plans already cover conventional LOD, asset graph/cook/package rows, byte-range page loading, background dispatch, caller-owned persistent background service state, safe-point adoption, streamed page GPU upload, streamed backend draw planning, D3D12/Vulkan GPU culling, D3D12/Vulkan indirect execution, DirectStorage system-memory adapter execution, and selected measured async-overlap performance proof.

This specification only defines the remaining advanced evidence gate:

- `mavg_mesh_shader_lod_ready`
- `mavg_mesh_shader_lod_d3d12_ready`
- `mavg_mesh_shader_lod_vulkan_ready`
- `mavg_metal_mesh_lod_ready`
- `mavg_package_visible_backend_readiness_ready`
- `mavg_autonomous_streaming_scheduler_ready`
- `mavg_async_overlap_measured_performance_ready`
- `mavg_deformation_integration_ready`
- `mavg_ray_tracing_integration_ready`
- `mavg_broad_cpu_gpu_memory_optimization_ready`
- `mavg_nanite_comparison_report_ready`

The gate must not change `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan`. It leaves the branch-selected active plan unchanged.

## Source Gate

Every positive advanced evidence row depends on a fresh source gate:

- `source_gate_date_yyyy_mm_dd`
- `source_gate_current_date_yyyy_mm_dd`
- `source_gate_max_age_days`
- `context7_vulkan_docs_ready`
- `context7_cmake_ready`
- `context7_vcpkg_ready`
- `official_d3d12_mesh_shader_ready`
- `official_vulkan_mesh_shader_ready`
- `official_apple_metal_ready`
- `official_directstorage_ready`
- `official_nanite_docs_ready`
- `official_profiler_docs_ready`

The source gate date records when the implementation owner refreshed the source matrix. It must be valid calendar `YYYY-MM-DD` and within the configured age window. Per-source document date fields record official-page metadata or inspected document dates; they must be valid calendar dates, but an older official document date is not itself stale if the source gate refresh is current.

Official-source date fields:

- `official_d3d12_mesh_tier_doc_date`
- `official_vulkan_mesh_ext_doc_date`
- `official_metal_feature_table_date`
- `official_pix_doc_date`
- `official_nsight_doc_date`
- `official_rgp_doc_date`
- `official_intel_gpa_doc_date`

## Official Constraints

D3D12 mesh shader rows must query `D3D12_FEATURE_DATA_D3D12_OPTIONS7::MeshShaderTier` and require `D3D12_MESH_SHADER_TIER_1` or better before D3D12 mesh execution can be ready. DirectX meshlet/dynamic LOD evidence must execute mesh/amplification shader bind points; a conventional indexed fallback cannot promote the mesh row.

Vulkan mesh shader rows must require `VK_EXT_mesh_shader`, `VkPhysicalDeviceMeshShaderFeaturesEXT::meshShader`, relevant `VkPhysicalDeviceMeshShaderPropertiesEXT` limits, and explicit `vkCmdDrawMeshTasksEXT` execution evidence. Indirect and indirect-count mesh dispatch are separate subclaims and require indirect-buffer usage, valid command layout, and synchronization evidence.

Metal mesh/Object shader rows are Apple-host-only. They must use Apple Metal feature-family evidence and native Metal resource preparation and must not infer readiness from D3D12 or Vulkan.

DirectStorage rows are IO-backend rows only. DirectStorage availability does not prove autonomous scheduling, GPU upload overlap, GPU destinations, GDeflate, or performance gain.

Ray tracing rows must prove acceleration-structure build/update policy and backend acceleration-structure execution evidence. Raster output or visual similarity cannot satisfy ray tracing readiness.

Performance and broad optimization rows require measured internal counters plus reviewed host artifacts from official or vendor-supported tools for the exact backend/vendor/workload row. One vendor, one backend, or one sample cannot promote broad readiness.

## Nanite Boundary

Nanite is a public Unreal Engine feature with documented virtualized geometry behavior, internal compressed mesh format, fine-grained streaming, automatic LOD, fallback behavior, and ray tracing fallback considerations. MAVG may produce a clean-room comparison report against public feature taxonomy.

This specification permanently blocks these product claims in the Task 1 matrix:

- `mavg_nanite_compatible`
- `mavg_nanite_equivalent`
- `mavg_nanite_superior`

Those fields remain false even when `mavg_nanite_comparison_report_ready` is true. A future legal, benchmark, and public wording review plan is required before any compatibility, equivalence, replacement, or superiority wording can be introduced elsewhere.

## Evaluation Rules

`evaluate_mavg_advanced_backend_evidence` is value-only. It must not execute backends, load packages, mutate resident state, expose native handles, infer cross-backend readiness, or read host profiler artifacts.
This is the cross-backend inference guard for the advanced MAVG cluster.

Required behavior:

- Empty input fails closed.
- Invalid or stale source gates keep every advanced ready row false.
- Missing Context7 or official source rows keep every advanced ready row false.
- Missing task rows produce `missing_task_row` diagnostics naming row ids.
- `mavg_mesh_shader_lod_ready` is true only when D3D12, Vulkan, and Metal mesh LOD rows are all true and the source gate is ready.
- `mavg_advanced_backend_evidence_ready` is true only when the source gate and every required task row are ready and diagnostics are empty.
- Nanite compatibility, equivalence, and superiority requests always add diagnostics and remain false.
- Current active plan mutation requests always add diagnostics.

## Validation

Focused validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_advanced_backend_evidence_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev -R MK_runtime_rhi_mavg_advanced_backend_evidence_tests --output-on-failure
```

Closeout validation must also run agent-surface checks, public API checks, text/C++ format checks, and full `tools/validate.ps1` before publication because this specification adds a public C++ API.
