# 2026-06-21 MAVG Advanced Backend Evidence Closeout v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Close the currently unclaimed MAVG/LOD advanced backend evidence cluster without overlapping completed MAVG Runtime LOD, streaming, GPU upload, indirect execution, or environment-commercial plans.

**Architecture:** This is a candidate follow-on plan. It does not select itself as `currentActivePlan` until an operator intentionally selects this MAVG advanced evidence cluster. It consumes completed MAVG evidence rows, adds fail-closed claim gates, and only promotes backend/readiness rows after deterministic tests or reviewed host artifacts prove the exact backend, platform, and workload named by the row.

**Tech Stack:** C++23, `MK_assets`, `MK_runtime`, `MK_runtime_rhi`, `MK_runtime_scene_rhi`, `MK_rhi_d3d12`, `MK_rhi_vulkan`, optional Apple-host `MK_rhi_metal`, CMake/CTest, PowerShell 7 validators, vcpkg manifest features, PIX on Windows, NVIDIA Nsight Graphics, AMD Radeon GPU Profiler.

---

**Plan ID:** `mavg-advanced-backend-evidence-closeout-v1`

## Non-Overlap Decision

This plan is intentionally separate from the completed `2026-06-05` and `2026-06-11` MAVG plans. Those plans already cover conventional LOD, MAVG asset graph/cook/package rows, byte-range page loading, background load dispatch, persistent background service state, safe-point adoption, streamed page upload, streamed backend draw planning, D3D12/Vulkan GPU culling, and D3D12/Vulkan compute-generated indirect consumption.

This plan only targets the remaining unclaimed cluster:

- Mesh-shader-based MAVG LOD execution.
- Metal-host readiness for MAVG mesh/LOD evidence.
- Deformation and ray tracing integration gates.
- Fully autonomous package streaming scheduler.
- Measured async-overlap performance proof, not just timing-window overlap.
- MAVG-only CPU/GPU/memory optimization evidence.
- Broad package-visible MAVG backend readiness aggregation.
- Nanite comparison taxonomy and benchmark evidence, without claiming Nanite format compatibility, equivalence, or superiority.

The branch-selected active plan stays active. This file is a candidate implementation plan and must not modify `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` during plan creation.

## Authoritative Source Gate

The implementation owner must refresh these sources before further code edits whenever more than 14 days have passed since the latest dated source revalidation note below.

| Source | Verified planning constraint |
| --- | --- |
| Context7 `/khronosgroup/vulkan-docs` | Vulkan indirect buffers must use indirect-buffer usage, 4-byte alignment, explicit synchronization, and `VK_EXT_mesh_shader` / synchronization2 feature checks before execution rows can be claimed. |
| Context7 `/kitware/cmake` | New C++ targets use `target_compile_features(... cxx_std_23)`, CMake file sets where appropriate, and focused CTest filters instead of ad hoc build surfaces. |
| Context7 `/microsoft/vcpkg` | Optional SDK dependencies stay in manifest features; `VCPKG_MANIFEST_INSTALL=OFF` remains the configure contract; bootstrap remains owned by `tools/bootstrap-deps.ps1`. |
| Microsoft DirectX Mesh Shader spec, https://microsoft.github.io/DirectX-Specs/d3d/MeshShader.html | D3D12 mesh shaders replace the IA vertex/index model for the mesh pipeline, use amplification/mesh shader bind points, and require a meshlet-oriented toolchain plus fallback path. |
| Microsoft Direct3D 12 Mesh Shader Samples, https://learn.microsoft.com/en-us/samples/microsoft/directx-graphics-samples/d3d12-mesh-shader-samples-win32/ | D3D12 mesh shader execution evidence requires GPU/driver support for DirectX 12 Ultimate; dynamic LOD selection through amplification shaders is an officially documented sample category. |
| Microsoft `D3D12_FEATURE_DATA_D3D12_OPTIONS7`, https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_feature_data_d3d12_options7 | D3D12 mesh readiness must query `MeshShaderTier`; `D3D12_MESH_SHADER_TIER_NOT_SUPPORTED` is a host-gated blocker. |
| Microsoft `D3D12_MESH_SHADER_TIER`, https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_mesh_shader_tier | `D3D12_MESH_SHADER_TIER_1` is the minimum positive D3D12 mesh/amplification shader support row. |
| Microsoft DirectX Mesh Shader pipeline statistics notes, https://microsoft.github.io/DirectX-Specs/d3d/MeshShader.html#pipeline-statistics | Pipeline-statistics counters are supplemental only. D3D12 mesh execution readiness must not depend on statistics fields unless the active Windows SDK headers and `CheckFeatureSupport` path expose the documented support query on the host; readback/hash execution remains the primary proof. |
| Khronos `VK_EXT_mesh_shader`, https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_mesh_shader.html | Vulkan mesh shader rows must query `VkPhysicalDeviceMeshShaderFeaturesEXT`, use the extension commands, and keep indirect/count variants feature-gated. |
| Khronos Mesh Shading for Vulkan, https://www.khronos.org/blog/mesh-shading-for-vulkan | Vulkan mesh shading is not automatically faster; the plan must preserve conventional pipeline fallback and require performance evidence before optimization claims. |
| Apple Metal LOD mesh shader sample, https://developer.apple.com/documentation/Metal/adjusting-the-level-of-detail-using-metal-mesh-shaders | The Apple-host MAVG mesh path must follow the documented object/mesh shader LOD pattern and prove selected-package output on Apple hardware. |
| Apple Metal sample code library, https://developer.apple.com/metal/sample-code/ | The official sample library describes Metal LOD mesh-shader execution as object/mesh shaders rendering meshlets and choosing point, line, or triangle LOD; implementation must map this only to first-party meshlet package rows. |
| Apple Metal mesh shaders, https://developer.apple.com/videos/play/wwdc2022/10162/ | Metal mesh/object shader readiness is Apple-host-only and can cover GPU-driven meshlet culling/LOD only after native Apple toolchain evidence. |
| Apple mesh/object shader resource commands, https://developer.apple.com/documentation/metal/mesh-and-object-shader-resource-preparation-commands | Metal readiness must bind object/mesh shader resources through Metal resource preparation commands instead of adapting D3D12/Vulkan native handles. |
| Apple Metal Feature Set Tables, https://developer.apple.com/metal/Metal-Feature-Set-Tables.pdf | Metal mesh/ray-tracing capability rows must be tied to feature-family limits and not inferred from D3D12/Vulkan results; render-pipeline ray tracing is not compatible with mesh shading and must be modeled as a separate pass or separate subclaim. |
| Microsoft DirectStorage, https://learn.microsoft.com/en-us/windows/win32/dstorage/dstorage-portal | DirectStorage is for high-throughput small asset reads with low CPU overhead; scheduler claims must distinguish filesystem, DirectStorage caller-owned executor, and any future first-party SDK adapter. |
| Microsoft DXR spec, https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html | D3D12 ray tracing rows must be acceleration-structure/pipeline evidence, not raster fallback evidence. |
| Microsoft `BuildRaytracingAccelerationStructure`, https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist4-buildraytracingaccelerationstructure | D3D12 ray tracing integration cannot be claimed without GPU acceleration-structure build evidence. |
| Vulkan Ray Tracing guide, https://docs.vulkan.org/guide/latest/extensions/ray_tracing.html | Vulkan ray tracing integration must use `VK_KHR_acceleration_structure` / ray tracing feature checks and opaque AS build rows. |
| Vulkan acceleration structures, https://docs.vulkan.org/spec/latest/chapters/accelstructures.html | Vulkan acceleration structure ownership, build/update, and synchronization are application responsibilities and must be explicit evidence rows. |
| Apple Metal ray tracing, https://developer.apple.com/videos/play/wwdc2023/10128/ | Metal ray tracing readiness is Apple-host-only and must remain separate from D3D12/Vulkan RT evidence. |
| Epic Nanite docs, https://dev.epicgames.com/documentation/unreal-engine/nanite-virtualized-geometry-in-unreal-engine | Nanite comparison axes are virtualized geometry, internal compressed format, fine-grained streaming, automatic LOD, high instance/detail scale, and fallback behavior. |
| Epic Nanite technical details, https://dev.epicgames.com/documentation/unreal-engine/nanite-technical-details | Fallback mesh behavior and ray tracing fallback policy are required comparison axes; they do not grant interoperability or equivalence claims. |
| PIX Timing Captures, https://devblogs.microsoft.com/pix/timing-captures-new/ | Async-overlap proof can use CPU/GPU/file IO timing artifacts and must preserve profiler overhead metadata. |
| Microsoft PIX timing captures, https://learn.microsoft.com/en-us/windows/win32/direct3dtools/pix/articles/timing-captures/pix-timing-captures | PIX Timing captures combine CPU and GPU profiling data and can show CPU cores, GPU submission latency, file IO, and allocations; the artifact schema must retain those dimensions. |
| NVIDIA Nsight Graphics GPU Trace, https://docs.nvidia.com/nsight-graphics/UserGuide/gpu-trace-overview.html | NVIDIA GPU evidence can use timestamp timelines for draw/dispatch duration and queue overlap; trace memory limits must be recorded. |
| AMD Radeon GPU Profiler manual, https://gpuopen.com/manuals/rgp_manual/ | AMD GPU evidence can use queue synchronization, async compute, barrier timing, and D3D12/Vulkan frame profiles; supported OS/API limits must be recorded. |
| Intel Graphics Performance Analyzers, https://www.intel.com/content/www/us/en/developer/tools/graphics-performance-analyzers/overview.html | Intel GPU evidence must record Intel GPA/GPA Framework availability, end-of-life/support status, API support, and tool version; if a supported official Intel capture path is unavailable, Intel rows remain host-gated. |
| Apple Metal developer tools, https://developer.apple.com/metal/tools/ | Metal performance proof must use Xcode/Metal debugger/Instruments or Metal counter evidence on Apple hosts; non-Apple profilers cannot promote Metal readiness. |

## 2026-06-22 Source Revalidation Notes

Context7 was rechecked for `/khronosgroup/vulkan-docs`, `/kitware/cmake`, and `/microsoft/vcpkg`. Vulkan documentation confirms `VkPhysicalDeviceMeshShaderFeaturesEXT`, `VkPhysicalDeviceMeshShaderPropertiesEXT`, `vkCmdDrawMeshTasksEXT`, `vkCmdDrawMeshTasksIndirectEXT`, and `vkCmdDrawMeshTasksIndirectCountEXT` as the relevant mesh-shader API surface. CMake documentation confirms target-local `target_sources(... FILE_SET ... TYPE CXX_MODULES ...)` and `target_compile_features(... cxx_std_23)` patterns. vcpkg documentation confirms optional feature rows and the `VCPKG_MANIFEST_INSTALL` switch used by this repository's bootstrap contract.

Official web documentation was also rechecked for D3D12 mesh shader tiers, DirectX meshlet samples, DirectStorage, DXR acceleration-structure builds, Vulkan ray tracing, Metal mesh/ray-tracing feature-family limits, PIX timing captures, Nsight GPU Trace, RGP, Intel GPA, and public Epic Nanite behavior. The plan remains source-backed, but implementation work must still refresh the table before Task 1 code edits if the 14-day freshness window has expired.

## Official-Recommended Implementation Rules

These rules convert official documentation into non-negotiable plan behavior:

- D3D12 mesh shader rows must call `CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, ...)`, record `D3D12_FEATURE_DATA_D3D12_OPTIONS7::MeshShaderTier`, and require `D3D12_MESH_SHADER_TIER_1` or better before any D3D12 mesh execution row can be ready.
- D3D12 mesh execution must build a mesh/amplification shader pipeline state and execute a selected first-party meshlet workload. A conventional indexed fallback can remain available, but fallback output cannot promote `mavg_mesh_shader_lod_d3d12_ready`.
- D3D12 mesh pipeline statistics can be recorded only as supplemental evidence after the active SDK headers and the host feature query prove support. The readiness row must remain provable by command execution plus deterministic readback/hash even when pipeline-statistics counters are unavailable.
- Vulkan mesh shader rows must require `VK_EXT_mesh_shader`, `VkPhysicalDeviceMeshShaderFeaturesEXT::meshShader`, and `VkPhysicalDeviceMeshShaderPropertiesEXT`; `vkCmdDrawMeshTasksEXT` is the base execution proof, while `vkCmdDrawMeshTasksIndirectEXT` and `vkCmdDrawMeshTasksIndirectCountEXT` are separate subclaims gated by the documented indirect/count support.
- Vulkan synchronization must use the repository's synchronization2 path for compute/page-upload-to-mesh-shader consumption and readback. Missing barriers are correctness failures, not performance defects.
- Metal mesh readiness must be proven only on an Apple host with Xcode/Metal tooling and documented feature-family support. Metal object/mesh shader resource binding must use Metal APIs, not backend-native D3D12/Vulkan handles. Metal mesh shading and Metal ray tracing must not be claimed as one combined render-pipeline feature when the feature tables require separate compatibility handling.
- cross-backend inference is forbidden. D3D12, Vulkan, and Metal rows each require their own backend/host evidence and cannot satisfy one another.
- DirectStorage rows must remain IO-backend rows. DirectStorage availability alone does not prove autonomous scheduling, GPU upload overlap, or performance gain.
- Ray tracing rows must prove acceleration-structure input policy and backend acceleration-structure build execution. Raster rendering, fallback mesh draw, or visual similarity is not ray tracing integration evidence.
- Performance proof must contain both internal timestamp/counter data and an external or reviewed host artifact reference. A timing-window overlap row is necessary context, but insufficient for speedup.
- Broad optimization rows are MAVG-scoped only. They cannot promote renderer-wide, environment-wide, or commercial readiness.
- Nanite rows are comparison-only. The implementation must not copy Unreal Engine internals, consume private Nanite formats, or use Nanite wording as a product claim.

## Plan Precision Gate

Before implementing any task in this plan, tighten that task to the same precision as the active slice:

- List every new public or backend-private type, function, validator output field, and static-check literal before code edits.
- Name every CTest target and the focused `ctest -R` filter that must pass for the slice.
- Name every negative test that prevents false readiness promotion.
- Define the exact ready field that can become true, and the exact `host_gated` or `unsupported` fields that must stay true or false when required hardware, OS, SDK, driver, profiler, or feature bits are missing.
- Treat backend fallback as a separate evidence row. A fallback row can prove graceful degradation but can never promote a mesh-shader, ray-tracing, Metal, scheduler, async-overlap, broad-optimization, or Nanite-comparison readiness row.
- Update this plan, manifest fragments, composed manifest, docs, schemas, and static guards in the same PR whenever an implementation discovers a sharper boundary than the original plan text.

## Known Blockers Are Rows, Not Assumptions

The implementation must not infer or hand-wave these conditions:

| Condition | Required handling |
| --- | --- |
| Host lacks D3D12 mesh shader tier support | Emit `mavg_mesh_shader_lod_d3d12_host_gated=1` and keep `mavg_mesh_shader_lod_d3d12_ready=0`. |
| Vulkan device lacks `VK_EXT_mesh_shader` | Emit `mavg_mesh_shader_lod_vulkan_host_gated=1` and keep `mavg_mesh_shader_lod_vulkan_ready=0`. |
| Apple host or Xcode/Metal feature-family evidence is unavailable | Emit `mavg_metal_mesh_lod_host_gated=1` and keep every Metal MAVG ready row false. |
| DXR or Vulkan acceleration-structure features are unavailable | Keep backend ray-tracing execution false and preserve policy-only diagnostics. |
| Profiler artifacts are unavailable or capture overhead changes behavior | Keep `mavg_async_overlap_measured_performance_ready=0` and record `profiler_artifact_blocked` with host/tool/version fields. |
| Broad vendor/backend matrix is incomplete | Keep `mavg_broad_cpu_gpu_memory_optimization_ready=0`; do not replace missing classes with one vendor or one backend. |
| Nanite legal/format interop is unresolved | Keep `mavg_nanite_compatible=0`, `mavg_nanite_equivalent=0`, and `mavg_nanite_superior=0`. |

## Claim Boundaries

These rows stay false until their exact tasks pass:

- `mavg_mesh_shader_lod_ready=0`
- `mavg_mesh_shader_lod_d3d12_ready=0`
- `mavg_mesh_shader_lod_vulkan_ready=0`
- `mavg_metal_mesh_lod_ready=0`
- `mavg_package_visible_backend_readiness_ready=1` only for the selected Task 2 package-visible closeout row after `tools/validate-mavg-backend-readiness.ps1 -RequireReady`; this is not a broad backend readiness claim.
- `mavg_autonomous_streaming_scheduler_ready=0`
- `mavg_async_overlap_measured_performance_ready=0`
- `mavg_deformation_integration_ready=0`
- `mavg_ray_tracing_integration_ready=0`
- `mavg_broad_cpu_gpu_memory_optimization_ready=0`
- `mavg_nanite_comparison_report_ready=0`

These rows are excluded from this plan's positive claim set:

- `mavg_nanite_compatible=0`
- `mavg_nanite_equivalent=0`
- `mavg_nanite_superior=0`

Reason: Epic Nanite is an Unreal Engine feature with an internal mesh format and rendering technology. This plan can create a clean-room comparison report against documented public behavior, but it must not claim file-format compatibility, replacement equivalence, or superiority. A future legal/benchmark/marketing claim plan is required before those words appear in package-visible readiness output.

## Task 1: Central Advanced Claim Matrix

**Files:**
- Create: `docs/specs/2026-06-21-mavg-advanced-backend-evidence-v1.md`
- Create: `engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_advanced_backend_evidence.hpp`
- Create: `engine/runtime_rhi/src/mavg_advanced_backend_evidence.cpp`
- Modify: `engine/runtime_rhi/CMakeLists.txt`
- Test: `tests/unit/runtime_rhi_mavg_advanced_backend_evidence_tests.cpp`

- [x] Write failing tests for an empty evidence matrix: every ready flag is false, every Nanite compatibility/equivalence/superiority flag is false, and diagnostics name each missing task row.
- [x] Define `MavgAdvancedBackendEvidenceDesc`, `MavgAdvancedBackendEvidenceResult`, and `evaluate_mavg_advanced_backend_evidence`.
- [x] Add source-gate fields for `source_gate_date_yyyy_mm_dd`, `context7_vulkan_docs_ready`, `context7_cmake_ready`, `context7_vcpkg_ready`, `official_d3d12_mesh_shader_ready`, `official_vulkan_mesh_shader_ready`, `official_apple_metal_ready`, `official_directstorage_ready`, `official_nanite_docs_ready`, and `official_profiler_docs_ready`.
- [x] Add per-source document-date fields for `official_d3d12_mesh_tier_doc_date`, `official_vulkan_mesh_ext_doc_date`, `official_metal_feature_table_date`, `official_pix_doc_date`, `official_nsight_doc_date`, `official_rgp_doc_date`, and `official_intel_gpa_doc_date`; missing or invalid fields keep every advanced ready row false. The source gate date, not the official document date, carries the freshness window.
- [x] Require all source-gate fields before any advanced ready row can be true.
- [x] Add tests for stale source gates, invalid calendar source dates, missing official source rows, accidental Nanite positive claims, and current-active-plan mutation requests.
- [x] Run focused validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_advanced_backend_evidence_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev -R MK_runtime_rhi_mavg_advanced_backend_evidence_tests --output-on-failure
```

Focused validation evidence on 2026-06-22: RED failed on invalid calendar source gate date before `parse_yyyy_mm_dd` was tightened; GREEN then passed `MK_runtime_rhi_mavg_advanced_backend_evidence_tests` with 1/1 CTest passing.

## Task 2: Package-Visible Backend Readiness Aggregator

**Files:**
- Create: `engine/runtime_scene_rhi/include/mirakana/runtime_scene_rhi/mavg_backend_readiness_closeout.hpp`
- Create: `engine/runtime_scene_rhi/src/mavg_backend_readiness_closeout.cpp`
- Modify: `engine/runtime_scene_rhi/CMakeLists.txt`
- Create: `tools/validate-mavg-backend-readiness.ps1`
- Test: `tests/unit/runtime_scene_rhi_mavg_backend_readiness_closeout_tests.cpp`

- [x] Add failing tests that require all selected package-visible rows before `mavg_package_visible_backend_readiness_ready=true`: graph cook/package row, runtime LOD selection row, resident page row, safe-point adoption row, streamed GPU upload row, streamed backend draw row, D3D12 compute-generated indirect consumption row, Vulkan compute-generated indirect consumption row, package smoke counter row, and zero native-handle diagnostics.
- [x] Reject Metal inference. Metal rows can be carried as `host_gated`, but they cannot satisfy D3D12/Vulkan rows and cannot promote broad backend readiness.
- [x] Reject value-only or planner-only rows where execution evidence is required.
- [x] Add `tools/validate-mavg-backend-readiness.ps1 -RequireReady`, which builds the test target and checks package counter text for `mavg_package_visible_backend_readiness_ready=1`.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-mavg-backend-readiness.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-mavg-backend-readiness.ps1 -RequireReady
```

Focused validation evidence on 2026-06-22: RED failed first on the missing `mavg_backend_readiness_closeout.hpp` contract, then GREEN passed `MK_runtime_scene_rhi_mavg_backend_readiness_closeout_tests` plus retained `MK_runtime_scene_rhi_mavg_streamed_backend_draw_tests`. `tools/validate-mavg-backend-readiness.ps1 -RequireReady` built `sample_desktop_runtime_game`, executed `--require-mavg-backend-readiness`, and emitted `mavg_package_visible_backend_readiness_ready=1`, nine required rows, ready rows `9`, diagnostics `0`, native handles `0`, Metal inference `0`, and broad backend readiness `0`.

Public contract evidence: `MavgBackendReadinessCloseoutDesc`, `MavgBackendReadinessCloseoutResult`, and `evaluate_mavg_backend_readiness_closeout` expose the selected package-visible row while preserving fail-closed diagnostics for missing rows, duplicate rows, non-ready package smoke, value-only execution gaps, native handle access, and Metal inference.

## Task 3: D3D12 Mesh Shader LOD Execution

**Task label:** MAVG Advanced Backend Evidence Closeout v1 Task 3

**Files:**
- Create: `engine/renderer/include/mirakana/renderer/mavg_mesh_shader_lod.hpp`
- Create: `engine/renderer/src/mavg_mesh_shader_lod.cpp`
- Create: `engine/rhi/d3d12/src/d3d12_mavg_mesh_shader_lod.cpp`
- Create or modify shaders under: `shaders/d3d12/`
- Modify: `engine/rhi/d3d12/CMakeLists.txt`
- Test: `tests/unit/mavg_mesh_shader_lod_tests.cpp`
- Test: `tests/unit/d3d12_mavg_mesh_shader_lod_tests.cpp`

- [x] Add a backend-neutral `MavgMeshShaderLodPlan` that converts selected MAVG clusters into meshlet task rows with deterministic fallback to conventional indexed draws.
- [x] Add D3D12 capability detection that requires mesh shader support and records `d3d12_mesh_shader_supported`, shader model rows, adapter name, and diagnostic text.
- [x] Query `D3D12_FEATURE_D3D12_OPTIONS7` through `ID3D12Device::CheckFeatureSupport`, store `MeshShaderTier`, and reject `D3D12_MESH_SHADER_TIER_NOT_SUPPORTED` before pipeline creation.
- [x] Implement a host-gated D3D12 execution path that dispatches amplification/mesh shader work for a first-party D3D12 meshlet workload and captures deterministic readback color/hash evidence; package-scene promotion remains outside Task 3.
- [x] Prove the mesh-shader row uses mesh/amplification shader bind points and has no IA input layout or index-buffer dependency in the mesh execution row. Keep `DispatchMesh` direct execution and any `ExecuteIndirect` mesh dispatch as separate subclaims.
- [x] Keep pipeline statistics supplemental: Task 3 records them as `pipeline_statistics_host_gated` / unavailable without blocking readback/hash execution evidence.
- [x] Keep `mavg_mesh_shader_lod_d3d12_ready=false` when the host lacks DirectX 12 Ultimate mesh shader support; missing hardware is a host-gated blocker, not a test pass.
- [x] Add tests for unsupported hardware, fallback preservation, wrong meshlet group size, mismatched material roots, invalid task rows, diagnostic text, and no index-buffer IA usage in the mesh-shader row.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_mesh_shader_lod_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev -R "MK_mavg_mesh_shader_lod_tests|MK_d3d12_mavg_mesh_shader_lod_tests" --output-on-failure
```

Focused validation evidence on 2026-06-22: RED failed first on the missing `mavg_mesh_shader_lod.hpp` and `d3d12_mavg_mesh_shader_lod.hpp` contracts, then GREEN passed `MK_mavg_mesh_shader_lod_tests` and `MK_d3d12_mavg_mesh_shader_lod_tests`. `D3d12MavgMeshShaderLodDispatchResult` records the D3D12 execution proof: it probes `D3D12_FEATURE_D3D12_OPTIONS7` / `MeshShaderTier`, compiles AS/MS/PS DXIL targets `as_6_5`, `ms_6_5`, and `ps_6_0` through `dxcompiler.dll` when available, creates a mesh pipeline-state stream without an IA input layout, calls direct `DispatchMesh`, copies a 64x64 render target to readback, and sets `mavg_mesh_shader_lod_d3d12_ready` only when readback evidence succeeds. Unsupported hardware, missing DXC, failed compile/PSO/readback, and invalid task rows remain fail-closed or host-gated with diagnostic text.

Claim boundary: Task 3 adds a D3D12-local execution proof API and backend-neutral meshlet task planner only. It does not add package-visible `mavg_mesh_shader_lod_ready=1`, Vulkan execution, Metal readiness, Nanite compatibility/equivalence/superiority, deformation, ray tracing, autonomous streaming scheduling, or broad CPU/GPU/memory optimization.

## Task 4: Vulkan Mesh Shader LOD Execution

**Files:**
- Create: `engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_mavg_mesh_shader_lod.hpp`
- Create: `engine/rhi/vulkan/src/vulkan_mavg_mesh_shader_lod.cpp`
- Create: `tests/shaders/vulkan_mavg_mesh_shader_lod.task.hlsl`
- Create: `tests/shaders/vulkan_mavg_mesh_shader_lod.mesh.hlsl`
- Create: `tests/shaders/vulkan_mavg_mesh_shader_lod.frag.hlsl`
- Modify: `engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_backend.hpp`
- Modify: `engine/rhi/vulkan/src/vulkan_backend.cpp`
- Modify: `engine/rhi/vulkan/CMakeLists.txt`
- Modify: root `CMakeLists.txt`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Test: `tests/unit/backend_scaffold_tests.cpp`
- Test: `tests/unit/vulkan_mavg_mesh_shader_lod_tests.cpp`
- Static guard: `tools/check-ai-integration-116-mavg-vulkan-mesh-shader-lod.ps1`

**2026-06-22 feature-enable slice evidence:** `VulkanPhysicalDeviceCandidate` now carries `supports_mesh_shader_extension`, `mesh_shader_supported`, `task_shader_supported`, and `mesh_shader_queries_supported`; `VulkanLogicalDeviceCreateDesc` adds `require_mesh_shader`, `require_task_shader`, and `enable_mesh_shader_queries`; `VulkanLogicalDeviceCreatePlan` adds `mesh_shader_enabled`, `task_shader_enabled`, and `mesh_shader_queries_enabled`. `build_logical_device_create_plan` now fail-closes when `VK_EXT_mesh_shader` is unavailable or requested mesh/task/query features are unsupported, appends `VK_EXT_mesh_shader` only when required, and `vkCreateDevice` uses `make_native_mesh_shader_features` plus `chain_native_device_features` so `VkPhysicalDeviceMeshShaderFeaturesEXT` is part of `VkDeviceCreateInfo.pNext` without dropping the Vulkan 1.3 dynamic-rendering/synchronization2 feature chain. `vulkan_device_command_requests` requires `vkCmdDrawMeshTasksEXT` and `vkCmdDrawMeshTasksIndirectEXT` only when mesh shaders are enabled. `probe_vulkan_mavg_mesh_shader_lod_capability()` now derives `mesh_shader_enabled`, `task_shader_enabled`, and direct/indirect mesh draw command availability from the logical-device plan/runtime device instead of treating support bits as enablement. Focused evidence: `MK_backend_scaffold_tests` and `MK_mavg_vulkan_mesh_shader_lod_tests` pass locally. This slice still does not implement mesh shader pipeline creation, `vkCmdDrawMeshTasksEXT` readback proof, indirect/count execution, Metal readiness, Nanite claims, broad MAVG backend readiness, or broad optimization.

- [ ] Add backend-private API `probe_vulkan_mavg_mesh_shader_lod_capability()` and `execute_vulkan_mavg_mesh_shader_lod(...)`; do not expose native Vulkan handles through public RHI or gameplay APIs.
- [ ] Capability probe must record `loader_available`, `instance_created`, `physical_device_selected`, `device_extension_supported`, `feature_query_executed`, `mesh_shader_supported`, `task_shader_supported`, `mesh_shader_queries_supported`, `mesh_shader_enabled`, `task_shader_enabled`, `draw_indirect_count_supported`, `draw_indirect_count_enabled`, `draw_mesh_tasks_direct_command_available`, `draw_mesh_tasks_indirect_command_available`, `draw_mesh_tasks_indirect_count_command_available`, `max_task_work_group_count_x`, `max_task_work_group_count_y`, `max_task_work_group_count_z`, `max_task_work_group_total_count`, `max_mesh_work_group_count_x`, `max_mesh_work_group_count_y`, `max_mesh_work_group_count_z`, `max_mesh_work_group_total_count`, `max_mesh_output_vertices`, `max_mesh_output_primitives`, adapter/vendor/device ids, and diagnostic text.
- [ ] Readiness requires `VK_EXT_mesh_shader`, `VkPhysicalDeviceMeshShaderFeaturesEXT::meshShader == VK_TRUE`, a logical-device feature chain that enables `meshShader`, and queried `VkPhysicalDeviceMeshShaderPropertiesEXT` limits that cover the selected first-party meshlet workload. If `taskShader == VK_FALSE` or is not enabled, direct mesh-only execution can be tested, but task-shader LOD amplification remains `host_gated`.
- [ ] Implement `vkCmdDrawMeshTasksEXT` execution for a selected first-party meshlet package row and require deterministic readback color/hash evidence before `mavg_mesh_shader_lod_vulkan_ready=true`.
- [ ] Prove the execution row uses `VK_SHADER_STAGE_MESH_BIT_EXT` and, when task amplification is enabled, `VK_SHADER_STAGE_TASK_BIT_EXT`; prove no vertex input layout, no index buffer binding, and no conventional indexed indirect draw promoted the mesh-shader row.
- [ ] Add `vkCmdDrawMeshTasksIndirectEXT` only when the command buffer uses a buffer created with `VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT`, offset is 4-byte aligned, `stride >= sizeof(VkDrawMeshTasksIndirectCommandEXT)`, `stride` is 4-byte aligned, `offset + stride * (drawCount - 1) + sizeof(VkDrawMeshTasksIndirectCommandEXT) <= buffer_size_bytes`, and draw counts stay within the queried per-axis and total mesh/task workgroup limits.
- [ ] Add `vkCmdDrawMeshTasksIndirectCountEXT` only when Vulkan 1.2, `VK_KHR_draw_indirect_count`, or `VK_AMD_draw_indirect_count` is supported, the `drawIndirectCount` feature is enabled, and `vkCmdDrawMeshTasksIndirectCountEXT` resolves from `vkGetDeviceProcAddr`; require both parameter and count buffers to have indirect-buffer usage, offsets to be 4-byte aligned, `maxDrawCount` to be bounded by the test workload, and the actual count to be captured in evidence. Otherwise set only `mavg_mesh_shader_lod_vulkan_indirect_count_host_gated=true`.
- [ ] Apply synchronization2 barriers for compute/page upload to mesh shader consumption and for readback evidence. Use `VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_EXT` and `VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT` when those stages consume data; missing or fallback-only barriers are correctness failures.
- [ ] Add tests named `vulkan_mavg_mesh_shader_lod_rejects_empty_task_rows`, `vulkan_mavg_mesh_shader_lod_probe_records_mesh_shader_feature_state`, `vulkan_mavg_mesh_shader_lod_host_gates_without_mesh_shader_support`, `vulkan_mavg_mesh_shader_lod_rejects_invalid_workgroup_counts`, `vulkan_mavg_mesh_shader_lod_rejects_indirect_range_overflow`, `vulkan_mavg_mesh_shader_lod_does_not_promote_fallback_indexed_draw`, and `vulkan_mavg_mesh_shader_lod_executes_mesh_shader_when_host_supports_it`.
- [ ] Test shader binaries are optional host evidence inputs: when the environment does not provide compiled SPIR-V paths, the execution test must assert a concrete skip/host-gated diagnostic rather than silently passing readiness. Shader source stays under `tests/shaders/` to match the repository's existing unit-test shader convention.
- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_vulkan_mesh_shader_lod_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev -R MK_mavg_vulkan_mesh_shader_lod_tests --output-on-failure
```

## Task 5: Fully Autonomous Package Streaming Scheduler

**Files:**
- Create: `engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_autonomous_streaming_scheduler.hpp`
- Create: `engine/runtime_rhi/src/mavg_autonomous_streaming_scheduler.cpp`
- Modify: `engine/runtime_rhi/CMakeLists.txt`
- Test: `tests/unit/runtime_rhi_mavg_autonomous_streaming_scheduler_tests.cpp`

- [ ] Define `RuntimeMavgAutonomousStreamingSchedulerState`, `RuntimeMavgAutonomousStreamingSchedulerDesc`, and `tick_runtime_mavg_autonomous_streaming_scheduler`.
- [ ] The scheduler must own candidate selection from view state, residency, page heat, budget pressure, IO availability, and safe-point policy. The caller still owns state storage and tick cadence.
- [ ] The scheduler must handle filesystem byte-range IO and the existing caller-owned DirectStorage executor as separate backend rows; it must not introduce global worker threads, global mounts, native handle exposure, or implicit package catalog mutation.
- [ ] Add tests for deterministic prioritization, duplicate coalescing, camera movement, memory pressure eviction, safe-point atomicity, cancelled package rows, failed IO rows, DirectStorage executor failure, and bounded per-frame work.
- [ ] `mavg_autonomous_streaming_scheduler_ready=true` only when the scheduler can select, dispatch, adopt, and evict pages over multiple frames without caller-precomputed page requests.
- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_autonomous_streaming_scheduler_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev -R MK_runtime_rhi_mavg_autonomous_streaming_scheduler_tests --output-on-failure
```

## Task 6: Measured Async-Overlap Performance Proof

**Files:**
- Create: `engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_async_overlap_performance.hpp`
- Create: `engine/runtime_rhi/src/mavg_async_overlap_performance.cpp`
- Create: `schemas/mavg-async-overlap-performance.schema.json`
- Create: `tools/validate-mavg-async-overlap-performance.ps1`
- Test: `tests/unit/runtime_rhi_mavg_async_overlap_performance_tests.cpp`

- [ ] Define a JSON artifact contract with workload id, package hash, adapter/backend, CPU frame timings, GPU timings, IO timings, queue overlap windows, memory budget, profiler tool metadata, and warmup/measurement frame counts.
- [ ] Require two comparable runs: `sync_baseline` and `async_scheduler`, same package hash, same camera script, same backend, same host class, 30 warmup frames, and 120 measured frames.
- [ ] Require internal engine timestamp/counter evidence from non-captured runs for threshold calculations, plus at least one reviewed external profiler artifact for overlap diagnosis. External profiler traces prove queue/IO/GPU overlap and memory behavior; they do not replace the non-captured timing threshold.
- [ ] Record profiler overhead, capture mode, capture duration, dropped/timestamp-overflow status, symbol/debug-info availability, tool version, driver version, and adapter id. If profiler overhead or missing trace data makes the comparison non-representative, keep `mavg_async_overlap_measured_performance_ready=false`.
- [ ] Promote `mavg_async_overlap_measured_performance_ready=true` only when all of these are true: p95 frame time improves by at least 5%, upload/streaming stall p95 improves by at least 20%, p99 frame time regresses by no more than 2%, visual replay hash matches, memory peak stays within budget, and profiler/timestamp evidence shows actual IO or CPU load overlap with GPU upload or draw work.
- [ ] Accept PIX, Nsight Graphics GPU Trace, or RGP artifact references as reviewed host evidence, but do not require any one vendor tool for all backends.
- [ ] Reject timing-window-only evidence from `mavg-streaming-upload-overlap-evidence-v1` as insufficient for performance proof.
- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-mavg-async-overlap-performance.ps1
```

## Task 7: Deformation Integration Gate

**Files:**
- Create: `engine/runtime_scene_rhi/include/mirakana/runtime_scene_rhi/mavg_deformation_integration.hpp`
- Create: `engine/runtime_scene_rhi/src/mavg_deformation_integration.cpp`
- Test: `tests/unit/runtime_scene_rhi_mavg_deformation_integration_tests.cpp`

- [ ] Define `MavgDeformationIntegrationDesc`, `MavgDeformationClusterBoundsRow`, and `plan_mavg_deformation_integrated_clusters`.
- [ ] Support only selected first-party deformation rows: rigid transforms, linear blend skinning with bounded joint influence metadata, and morph target rows with conservative cluster AABB expansion.
- [ ] Reject topology-changing deformation, runtime-generated triangle topology, and unbounded vertex displacement.
- [ ] Require stable cluster ids, updated conservative bounds, material root preservation, residency page validity, and fallback draw range preservation.
- [ ] Add D3D12/Vulkan execution tests only after backend-local dynamic vertex/payload upload rows exist; until then, keep execution subrows false and policy rows ready only.
- [ ] `mavg_deformation_integration_ready=true` requires both policy evidence and selected backend execution evidence for the named backend set.
- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_scene_rhi_mavg_deformation_integration_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev -R MK_runtime_scene_rhi_mavg_deformation_integration_tests --output-on-failure
```

## Task 8: Ray Tracing Integration Gate

**Files:**
- Create: `engine/runtime_scene_rhi/include/mirakana/runtime_scene_rhi/mavg_ray_tracing_integration.hpp`
- Create: `engine/runtime_scene_rhi/src/mavg_ray_tracing_integration.cpp`
- Test: `tests/unit/runtime_scene_rhi_mavg_ray_tracing_integration_tests.cpp`
- Optional backend files after value tests pass: `engine/rhi/d3d12/src/d3d12_mavg_ray_tracing_blas.cpp`, `engine/rhi/vulkan/src/vulkan_mavg_ray_tracing_blas.cpp`

- [ ] Define `MavgRayTracingGeometryPolicy`, `MavgRayTracingBlasInputRow`, and `plan_mavg_ray_tracing_blas_inputs`.
- [ ] Provide two explicit modes: resident-cluster BLAS input and fallback-mesh BLAS input. Never silently switch modes.
- [ ] Require stable replay hash, geometry byte count, index format, transform row, material alpha policy, and fallback mismatch diagnostics.
- [ ] D3D12 readiness requires DXR capability checks plus acceleration-structure build evidence from `BuildRaytracingAccelerationStructure`.
- [ ] Vulkan readiness requires `VK_KHR_acceleration_structure` feature checks plus acceleration-structure build evidence.
- [ ] Metal readiness for ray tracing is not inferred; it belongs to Task 9.
- [ ] `mavg_ray_tracing_integration_ready=true` requires policy evidence plus selected backend acceleration-structure build evidence.
- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_scene_rhi_mavg_ray_tracing_integration_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev -R MK_runtime_scene_rhi_mavg_ray_tracing_integration_tests --output-on-failure
```

## Task 9: Apple-Host Metal MAVG Readiness

**Files:**
- Create: `engine/rhi/metal/src/metal_mavg_mesh_lod.mm`
- Create or modify: `shaders/metal/`
- Create: `tools/check-mavg-metal-host-evidence.ps1`
- Modify: `.github/workflows/validate.yml` only if a reviewed macOS host lane is required for this plan.
- Test: Apple-host `MK_mavg_metal_mesh_lod_tests`

- [ ] Add Apple-host capability capture for Metal GPU family, mesh/object shader support, ray tracing support, Xcode version, macOS version, and feature set table row id.
- [ ] Implement selected MAVG meshlet LOD execution through Metal object/mesh shaders on Apple hardware that supports the row.
- [ ] Add a Metal ray-tracing policy bridge only after Task 8 value rows exist; do not claim Metal RT from D3D12/Vulkan proof, and do not model Metal mesh shading plus render-pipeline ray tracing as one pipeline when the feature tables mark that combination incompatible.
- [ ] Emit `mavg_metal_mesh_lod_ready=1` only from Apple-host execution evidence; Windows, Linux, and simulator-only evidence cannot promote it.
- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-mavg-metal-host-evidence.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-mavg-metal-host-evidence.ps1 -RequireReady
```

## Task 10: MAVG-Only Broad CPU/GPU/Memory Optimization Matrix

**Files:**
- Create: `schemas/mavg-broad-optimization-artifacts.schema.json`
- Create: `tools/validate-mavg-broad-optimization.ps1`
- Create: `engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_broad_optimization_evidence.hpp`
- Create: `engine/runtime_rhi/src/mavg_broad_optimization_evidence.cpp`
- Test: `tests/unit/runtime_rhi_mavg_broad_optimization_evidence_tests.cpp`

- [ ] Define required metrics: CPU frame p50/p95/p99, CPU streaming jobs p95, GPU frame p50/p95/p99, GPU upload p95, GPU draw/dispatch p95, VRAM peak, resident page bytes, page faults, IO bytes/sec, IO request count, heap allocation count, and replay hash.
- [ ] Require at least these backend/host classes before `mavg_broad_cpu_gpu_memory_optimization_ready=true`: D3D12 NVIDIA/AMD/Intel where available, Vulkan NVIDIA/AMD/Intel where available, and Apple Metal family row where available. Missing classes remain host-gated and block broad readiness unless the manifest explicitly excludes that platform row in a reviewed plan update.
- [ ] For each vendor/backend row, record the official profiler path used: PIX or vendor-supported alternative for D3D12, Nsight GPU Trace for NVIDIA where supported, RGP/RDP for AMD where supported, Intel GPA/GPA Framework or PIX for Intel D3D12 where supported, and Xcode/Metal debugger/Instruments/Metal counters for Apple Metal. If a current official tool is end-of-life, unsupported on the host/API, or unavailable, mark only that row host-gated and do not substitute another vendor's evidence.
- [ ] Require each backend class to pass the same first-party MAVG stress package and one first-party gameplay package.
- [ ] Reject single-vendor, single-backend, or synthetic-only results as broad optimization proof.
- [ ] Keep this row scoped to MAVG. It must not promote `environment_broad_optimization_ready`, `renderer_broad_optimization_ready`, or broad engine commercial readiness.
- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-mavg-broad-optimization.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-mavg-broad-optimization.ps1 -RequireReady
```

## Task 11: Nanite Comparison Report Without Nanite Claims

**Files:**
- Create: `docs/specs/2026-06-21-mavg-nanite-comparison-taxonomy-v1.md`
- Create: `tools/validate-mavg-nanite-comparison.ps1`
- Create: `schemas/mavg-nanite-comparison-report.schema.json`

- [ ] Define comparison axes from public Epic docs only: virtualized geometry, internal compressed data, fine-grained streaming, automatic LOD, instance/detail scale, fallback mesh behavior, ray tracing fallback behavior, data size, material limits, deformation limits, and platform fallback.
- [ ] Use only first-party MIRAIKANAI assets and generated test scenes. Do not use Unreal Engine source code, copied Nanite data formats, or Epic sample assets unless a separate legal review records the license and need.
- [ ] Emit `mavg_nanite_comparison_report_ready=1` when the report is complete and reproducible.
- [ ] Force `mavg_nanite_compatible=0`, `mavg_nanite_equivalent=0`, and `mavg_nanite_superior=0` in the validator output.
- [ ] If a future operator wants those claims, require a separate plan with legal review, public wording review, cross-engine benchmark methodology, and hosted artifact preservation.
- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-mavg-nanite-comparison.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-mavg-nanite-comparison.ps1 -RequireReady
```

## Task 12: Docs, Manifest, Static Guards, And Closeout

**Files:**
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/specs/2026-06-05-mavg-benchmark-methodology-v1.md`
- Modify: `docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Compose: `engine/agent/manifest.json`
- Modify or add: `schemas/engine-agent.schema.json`
- Modify or add: `tools/check-ai-integration-*.ps1`

- [ ] Update docs so every newly promoted row has a matching non-claim list.
- [ ] Update manifest fragments only after code/tests for that row pass; never hand-edit `engine/agent/manifest.json`.
- [ ] Add static needles for every public row id and every prohibited Nanite claim.
- [ ] Keep `currentActivePlan` unchanged unless this plan is intentionally selected for execution.
- [ ] Compose and validate:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
git diff --check
```

## Final Validation Gate

Run focused validators first, then one full slice gate after docs, manifest, static checks, schemas, and source gates settle:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

## Done When

- The plan is selected intentionally, or remains a documented candidate without changing active execution state.
- Every claimed row has deterministic tests or reviewed host artifacts.
- Missing host classes stay host-gated and block broad readiness instead of being inferred.
- Nanite compatibility/equivalence/superiority remain false.
- Docs, manifest fragments, composed manifest, schemas, static checks, and validation scripts agree with the exact claim boundaries.
- `tools/validate.ps1` passes, or a concrete host/tooling blocker is recorded in the plan and manifest without promoting readiness.
