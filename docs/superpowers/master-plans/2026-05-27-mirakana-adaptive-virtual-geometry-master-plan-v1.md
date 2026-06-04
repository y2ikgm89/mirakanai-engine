# Mirakana Adaptive Virtual Geometry Master Plan v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:writing-plans to create a focused dated child implementation plan before editing code. Child plans should then use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement task-by-task. Steps in child plans use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a clean-break, first-party virtualized clustered geometry system that can exceed Nanite-like static raster LOD by unifying static geometry, dynamic deformation, ray tracing, large-scene streaming, and temporal quality governance.

**Architecture:** Treat MAVG as a new geometry subsystem, not as a compatibility layer over conventional mesh LOD. Offline tools build a validated cluster graph; runtime systems select, stream, and render resident clusters through backend-neutral renderer/RHI contracts with D3D12, Vulkan, and host-gated Metal evidence. Every broad claim is fail-closed until backed by tests, package evidence, backend-local validation, profiling counters, docs, manifest rows, and legal/dependency review.

**Tech Stack:** C++23, `MK_assets`, `MK_tools`, `MK_renderer`, `MK_rhi`, `MK_runtime`, `MK_runtime_rhi`, `MK_runtime_scene_rhi`, `MK_scene_renderer`, CMake/CTest, PowerShell validation tools, D3D12 mesh shaders and indirect draws, Vulkan `VK_EXT_mesh_shader` and indirect draws, optional D3D12 Work Graphs research gates, Vulkan ray tracing acceleration-structure gates, GPU memory/residency policy, frame graph multi-queue execution, runtime package streaming, and clean-room public research inputs.

---

**Plan ID:** `mirakana-adaptive-virtual-geometry-master-plan-v1`

**Status:** Candidate long-range master plan. Not selected as `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan`.

**Date:** 2026-05-27

## Master Plan Decision

This should be a master plan, not a single active dated implementation plan.

Reasons:

- The work crosses asset import/cook, renderer, RHI, runtime streaming, scene rendering, profiling, validation, package evidence, legal review, and agent-surface contracts.
- It has multiple independently reviewable milestones that need their own API and validation boundaries.
- It should not displace the manifest-selected active plan or production-completion selection gate until the operator intentionally selects the first MAVG child plan.
- The "Nanite-exceeding" claim is a final integrated benchmark claim, not a useful early implementation target.

Use this file as the roadmap and decision contract. Create focused child plans under `docs/superpowers/plans/YYYY-MM-DD-mavg-<phase>.md` when executing a phase.

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

Use official SDK docs as constraints, not as copied implementation. Use the research papers for algorithmic ideas and write original implementations.

## Current Engine Anchors

Existing usable foundations:

- `engine/runtime/include/mirakana/runtime/entity_scale_culling.hpp` has value-only LOD band, visibility, draw/update cost, and budget planning.
- `engine/renderer/include/mirakana/renderer/scene_scale_policy.hpp` has backend-neutral scene scale, culling, batching, and LOD policy rows.
- `engine/runtime_rhi/include/mirakana/runtime_rhi/runtime_upload.hpp` has runtime mesh, skinned mesh, morph mesh, and texture upload evidence.
- `engine/renderer/include/mirakana/renderer/frame_graph_rhi.hpp` has multi-queue frame graph command submission, render-pass envelopes, barriers, transient texture lifetimes, and package evidence hooks.
- `engine/renderer/include/mirakana/renderer/gpu_memory_policy.hpp` has memory/residency policy evidence rows.
- `engine/rhi/include/mirakana/rhi/rhi.hpp` has backend-neutral stats, queues, buffers, textures, draw counters, and transient resource counters.
- D3D12 and Vulkan backends already carry selected package/evidence lanes; Metal remains Apple-host-gated.

Important gaps:

- No cooked cluster/meshlet asset format.
- No cluster hierarchy builder.
- No cluster simplification/error metric.
- No GPU cluster traversal or visibility buffer.
- No mesh shader or indirect clustered geometry path.
- No cluster page streaming/residency.
- No unified raster/ray tracing cluster payload.
- No deformation-safe cluster bounds or dynamic update policy.
- No benchmark harness capable of supporting a "beyond Nanite-like LOD" claim.

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

| Scope | Solo senior engineer | 3 senior engineers | Confidence |
| --- | ---: | ---: | --- |
| Static cluster asset graph plus CPU selection prototype | 4-6 months | 2-3 months | Medium |
| Static GPU clustered renderer with D3D12 and Vulkan evidence | 9-14 months | 5-8 months | Medium-low |
| Streaming, graceful degradation, profiling, and package proof | 14-22 months | 8-12 months | Medium-low |
| Deformable clusters plus RT integration | 22-36 months | 12-20 months | Low |
| Integrated "exceeds Nanite-like LOD on selected axes" benchmark claim | 30-48 months | 18-30 months | Low |

Minimum useful production-facing v1: 8-12 person-months.

Credible flagship system across all requested axes: 48-72 person-months, plus host validation and legal/FTO review.

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

- Create: `docs/specs/2026-05-27-mavg-architecture-v1.md`
- Create: `docs/specs/2026-05-27-mavg-benchmark-methodology-v1.md`
- Modify: `docs/dependencies.md`
- Modify: `docs/legal-and-licensing.md`
- Modify: `THIRD_PARTY_NOTICES.md` only if a dependency or dataset is selected.

**Deliverables:**

- Official-doc check matrix for D3D12, Vulkan, Metal, and ray tracing features.
- Clean-room record: what public sources are allowed, what is forbidden, and who reviewed it.
- Benchmark scenes selected from first-party/generated assets only.
- Baseline counters for conventional static mesh, skinned mesh, morph mesh, package streaming, memory, frame graph, and renderer quality.
- First child plan selection.

**Validation:**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
git diff --check
```

### Phase 1 - MAVG Asset Graph v1

**Estimate:** 8-12 weeks solo, 4-6 weeks with 3 engineers.

**Files:**

- Create: `engine/assets/include/mirakana/assets/mavg_cluster_graph.hpp`
- Create: `engine/assets/src/mavg_cluster_graph.cpp`
- Create: `engine/tools/include/mirakana/tools/mavg_cluster_cook.hpp`
- Create: `engine/tools/asset/src/mavg_cluster_cook.cpp`
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

**Estimate:** 8-12 weeks solo, 4-6 weeks with 3 engineers.

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

**Done when:**

- MAVG static geometry renders through the conventional backend path with package-visible counters and no public native handles.

### Phase 4 - GPU Culling And Indirect Draw v1

**Estimate:** 10-16 weeks solo, 6-9 weeks with 3 engineers.

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

**Estimate:** 10-16 weeks solo, 6-10 weeks with 3 engineers.

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

- Shader phases: `tools/check-shader-toolchain.ps1` plus DXIL/SPIR-V validation.
- Package phases: `tools/package-desktop-runtime.ps1` and installed runtime validation.
- Dependency phases: `tools/bootstrap-deps.ps1`, `tools/check-dependency-policy.ps1`, legal docs, and notices.
- Vulkan phases: strict validation-layer evidence and SPIR-V evidence.
- Metal phases: Apple-host evidence only.
- Benchmark phases: repeatable runs with raw output stored outside source or as intentionally tracked summarized evidence.

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
- `.claude/skills/gameengine-rendering/SKILL.md`
- `.cursor/skills/gameengine-rendering/SKILL.md`
- `tools/check-ai-integration*.ps1`
- Package sample `game.agent.json` files.

Do not update all surfaces mechanically. Update only the surfaces whose durable behavior, validation, or AI-operable contract changed.

## Risk Register

| Risk | Severity | Mitigation |
| --- | --- | --- |
| Patent/FTO issue around virtualized geometry | High | Clean-room design, avoid UE source, counsel review before commercial claim. |
| Scope explosion | High | Master plan plus focused child plans; no single child plan spans more than one API/validation boundary. |
| Mesh shader availability gaps | High | Keep compute/indirect fallback as production path until backend evidence exists. |
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

- `docs/superpowers/plans/YYYY-MM-DD-mavg-asset-graph-v1.md`

The first production-facing claim should be "static clustered geometry with deterministic fallback and package evidence", not "Nanite replacement".
