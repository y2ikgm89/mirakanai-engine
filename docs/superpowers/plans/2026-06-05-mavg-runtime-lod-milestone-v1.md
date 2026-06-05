# 2026-06-05 MAVG Runtime LOD Milestone v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` or focused inline TDD execution to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `mavg-runtime-lod-milestone-v1`

**Status:** Active.

**Execution State:** Selected as `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` after `mavg-asset-graph-v1`, `mavg-runtime-lod-selection-v1`, and `mavg-runtime-lod-graph-main-v1` merged to `main` through PR #516, PR #517, and PR #518. Implementation tasks remain open and must proceed phase-by-phase with focused validation and no broad MAVG/Nanite/backend readiness claims.

**Goal:** Implement the first visible static MAVG LOD path: deterministic hierarchy/error/fallback graph rows, deterministic CPU LOD selection, package-resident page awareness, conventional indexed draw-range support, and conventional renderer submission without GPU culling, mesh shaders, streaming IO execution, deformation, ray tracing, or Nanite-equivalence claims.

**Architecture:** Close the current asset-graph gaps first, then add a value-only `MK_renderer` CPU selector, then bridge selected rows into `MK_runtime` resident package/catalog evidence, and finally submit selected clusters through the existing conventional indexed mesh path in `MK_scene_renderer`. GPU-driven culling, indirect draw execution, mesh shaders, background streaming, deformation tiers, ray tracing, and benchmark superiority stay separate future child plans with their own backend/host evidence.

**Tech Stack:** C++23, `MK_assets`, `MK_tools`, `MK_renderer`, `MK_runtime`, `MK_runtime_rhi`, `MK_scene_renderer`, existing `MK_rhi` upload/frame-graph paths, CMake/CTest, PowerShell validation scripts, Context7-checked CMake/Vulkan docs, Microsoft Direct3D 12 docs, Khronos Vulkan docs, and Apple Metal docs.

---

## Current Project Audit

`mavg-asset-graph-v1` has implemented the first data-only foundation:

- `engine/assets/include/mirakana/assets/mavg_cluster_graph.hpp` exposes `MavgClusterGraphDocument`, `MavgClusterGraphCluster::lod_level`, page rows, material partitions, child ids, validation, canonicalization, text serialization, and text deserialization.
- `engine/tools/include/mirakana/tools/mavg_cluster_cook.hpp` exposes `MavgClusterCookRequest`, `plan_mavg_cluster_graph_cook_package`, and `apply_mavg_cluster_graph_cook_package`.
- The cook request now carries static `MavgClusterCookVertex` rows plus indexed `MavgClusterCookTriangle` rows and emits deterministic `GameEngine.MavgClusterPayload.v1` `vertex.data_hex` / `index.data_hex` rows.
- This milestone's first four implementation checkpoints now add graph parent ids, geometric error, resident fallback ancestors, cluster draw ranges, draw-ready static cook payload rows, a deterministic value-only `MK_renderer` CPU selector, and an `MK_runtime` resident-page evidence bridge. Renderer submission and package streaming execution remain future tasks in this same milestone.
- `MK_runtime` already has resident package mount sets, resident catalog caches, byte/record budget checks, selected safe-point package streaming, and reviewed eviction-assisted commit helpers.
- `MK_renderer` already has `MeshCommand`, `MeshGpuBinding`, `RendererStats`, `NullRenderer`, frame graph/RHI policies, GPU memory policy rows, and renderer quality evidence surfaces.
- `MeshCommand` and `rhi::IRhiCommandList::draw_indexed` do not yet expose a selected index range, so a visible conventional MAVG LOD path must add range-aware indexed draw support before scene submission can draw only selected clusters.
- `MK_scene_renderer` already converts `SceneRenderPacket` meshes into `MeshCommand` rows and submits them through `IRenderer::draw_mesh`.
- `MK_runtime_rhi` already uploads conventional mesh payloads with known vertex layouts and index buffers through `RuntimeMeshUploadResult`.
- `MK_rhi` has backend capability profiles and selected D3D12/Vulkan/Metal host gates, but no MAVG-specific indirect draw, mesh shader, or GPU cluster traversal API.

## Official Source Audit

Checked on 2026-06-05:

- Context7 `/kitware/cmake`: keep targets/test executables explicit, use CTest targets, and preserve repository CMake target organization.
- Context7 `/khronosgroup/vulkan-docs`: Vulkan mesh/task shader pipeline stages, indirect draw count, and feature-gated stage usage must not be claimed without enabled features and backend validation.
- Context7 Microsoft Direct3D 12 docs: `ExecuteIndirect` and GPU culling are relevant only after the CPU/reference selector and conventional draw fallback are proven.
- Microsoft Learn Direct3D 12 Indirect Drawing (`https://learn.microsoft.com/en-us/windows/win32/direct3d12/indirect-drawing`): command signatures define argument buffer layout, command type, and per-command resource binding changes; command buffers can be generated on CPU or GPU.
- Microsoft Learn `ID3D12GraphicsCommandList::ExecuteIndirect` (`https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist-executeindirect`): offsets are 4-byte aligned, argument/count buffers must be buffer resources, and execution belongs to direct or compute command lists.
- Microsoft DirectX Mesh Shader spec (`https://microsoft.github.io/DirectX-Specs/d3d/MeshShader.html`) and Microsoft mesh shader samples (`https://learn.microsoft.com/en-us/samples/microsoft/directx-graphics-samples/d3d12-mesh-shader-samples-win32/`): mesh/amplification shaders can support meshlet culling and GPU LOD, but feature queries and fallback paths are required before using them as a ready path.
- Khronos `VK_EXT_mesh_shader` (`https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_mesh_shader.html`) and `vkCmdDrawMeshTasksIndirectCountEXT` (`https://docs.vulkan.org/refpages/latest/refpages/source/vkCmdDrawMeshTasksIndirectCountEXT.html`): Vulkan mesh shader support is extension/feature/property-gated and indirect count reads draw counts from a device buffer.
- Apple Metal indirect command buffer docs (`https://developer.apple.com/documentation/metal/indirect-command-encoding`) and residency set docs (`https://developer.apple.com/documentation/metal/simplifying-gpu-resource-management-with-residency-sets`): Metal GPU-driven command execution and residency sets exist, but Metal readiness remains Apple-host-gated and must not be inferred from Windows validation.

## Scope

This milestone implements static conventional-renderer LOD readiness only.

In scope:

- Graph hierarchy/error/fallback rows needed to avoid holes.
- Draw payload/range metadata sufficient for conventional indexed draw fallback.
- CPU reference LOD selector in `MK_renderer`.
- Resident page set input and missing page request rows.
- Backend-neutral conventional indexed draw range support for D3D12, Vulkan, and Null RHI through existing opaque handles.
- Deterministic fallback substitution when fine clusters are not resident.
- Temporal hysteresis rows that prevent rapid LOD oscillation.
- Optional renderer-neutral scene submission planning over existing `MeshCommand` and existing `SceneRenderPacket` concepts.
- Package-visible counters and docs/manifest wording that claim only selected conventional static MAVG LOD readiness.

Out of scope:

- GPU culling.
- D3D12 `ExecuteIndirect` or Vulkan indirect draw execution.
- Mesh/task/amplification shaders.
- Background package streaming workers.
- New GPU allocator or residency enforcement.
- Deformation, skinning, morph clusters, ray tracing, occlusion culling, Work Graphs, Metal proof, or benchmark superiority.
- Nanite compatibility, Nanite-equivalent, or Nanite-exceeding claims.

## Files

- Modify: `engine/assets/include/mirakana/assets/mavg_cluster_graph.hpp`
- Modify: `engine/assets/src/mavg_cluster_graph.cpp`
- Modify: `engine/tools/include/mirakana/tools/mavg_cluster_cook.hpp`
- Modify: `engine/tools/asset/mavg_cluster_cook.cpp`
- Modify: `engine/renderer/CMakeLists.txt`
- Modify: `engine/renderer/include/mirakana/renderer/renderer.hpp`
- Modify: `engine/renderer/src/null_renderer.cpp`
- Modify: `engine/renderer/src/rhi_frame_renderer.cpp`
- Modify: `engine/renderer/src/rhi_postprocess_frame_renderer.cpp`
- Modify: `engine/renderer/src/rhi_directional_shadow_smoke_frame_renderer.cpp`
- Create: `engine/renderer/include/mirakana/renderer/mavg_lod_selection.hpp`
- Create: `engine/renderer/src/mavg_lod_selection.cpp`
- Modify: `engine/rhi/include/mirakana/rhi/rhi.hpp`
- Modify: `engine/rhi/src/null_rhi.cpp`
- Modify: `engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp`
- Modify: `engine/rhi/d3d12/src/d3d12_backend.cpp`
- Modify: `engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_backend.hpp`
- Modify: `engine/rhi/vulkan/src/vulkan_backend.cpp`
- Modify: `engine/runtime/CMakeLists.txt`
- Create: `engine/runtime/include/mirakana/runtime/mavg_lod_residency.hpp`
- Create: `engine/runtime/src/mavg_lod_residency.cpp`
- Modify: `engine/scene_renderer/CMakeLists.txt`
- Create: `engine/scene_renderer/include/mirakana/scene_renderer/mavg_scene_lod.hpp`
- Create: `engine/scene_renderer/src/mavg_scene_lod.cpp`
- Modify: `CMakeLists.txt`
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Compose: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration-010-agent-baseline.ps1`
- Modify: `tools/check-ai-integration-020-engine-manifest.ps1`
- Modify: `tools/check-json-contracts-030-tooling-contracts.ps1`
- Modify: `tools/check-json-contracts-040-agent-surfaces.ps1`
- Test: `tests/unit/mavg_cluster_graph_tests.cpp`
- Test: `tests/unit/tools_mavg_cluster_cook_tests.cpp`
- Create: `tests/unit/mavg_lod_selection_tests.cpp`
- Modify: `tests/unit/rhi_tests.cpp`
- Modify: `tests/unit/d3d12_rhi_tests.cpp`
- Modify: `tests/unit/backend_scaffold_tests.cpp`
- Modify: `tests/unit/renderer_rhi_tests.cpp`
- Create: `tests/unit/runtime_mavg_lod_residency_tests.cpp`
- Create: `tests/unit/scene_renderer_mavg_lod_tests.cpp`

## Public API Contract

`MavgClusterGraphCluster` is clean-break expanded to include draw and fallback metadata:

```cpp
struct MavgClusterGraphCluster {
    std::uint32_t cluster_index{0};
    std::uint32_t page_index{0};
    std::uint32_t local_cluster_index{0};
    std::uint32_t lod_level{0};
    std::uint32_t triangle_count{0};
    std::uint32_t vertex_count{0};
    MavgBounds3f bounds;
    std::uint32_t material_partition{0};
    std::uint32_t parent_cluster_index{0};
    bool has_parent{false};
    std::uint32_t resident_fallback_cluster_index{0};
    float geometric_error{0.0F};
    std::uint32_t first_index{0};
    std::uint32_t index_count{0};
    std::int32_t vertex_base{0};
    std::vector<std::uint32_t> children;
};
```

`engine/assets/include/mirakana/assets/mavg_cluster_graph.hpp` owns shared selector/residency rows used by both `MK_renderer` and `MK_runtime` without introducing a runtime-to-renderer dependency:

```cpp
namespace mirakana {

struct MavgLodResidentPageSet {
    std::vector<std::uint32_t> page_indices;
};

struct MavgLodPageRequest {
    AssetId graph_asset;
    std::uint32_t page_index{0};
    float priority{0.0F};
    std::string reason;
};

} // namespace mirakana
```

`engine/renderer/include/mirakana/renderer/mavg_lod_selection.hpp` owns the value-only selector:

```cpp
namespace mirakana {

enum class MavgLodSelectionDiagnosticCode : std::uint8_t {
    invalid_graph,
    invalid_view,
    missing_root_cluster,
    missing_resident_fallback,
    missing_page,
    budget_degraded,
    budget_unsatisfied,
    hysteresis_reused_previous,
};

struct MavgLodViewDesc {
    Vec3 camera_world_position{};
    float viewport_height_pixels{0.0F};
    float target_error_pixels{1.0F};
    float hysteresis_pixels{0.25F};
    std::uint32_t max_selected_clusters{0};
};

struct MavgLodPreviousSelection {
    std::vector<std::uint32_t> cluster_indices;
};

struct MavgLodSelectedCluster {
    AssetId graph_asset;
    std::uint32_t cluster_index{0};
    std::uint32_t page_index{0};
    std::uint32_t lod_level{0};
    std::uint32_t material_partition{0};
    std::uint32_t first_index{0};
    std::uint32_t index_count{0};
    std::int32_t vertex_base{0};
    bool fallback_substitution{false};
};

struct MavgLodSelectionDiagnostic {
    MavgLodSelectionDiagnosticCode code{MavgLodSelectionDiagnosticCode::invalid_graph};
    std::uint32_t cluster_index{0};
    std::uint32_t page_index{0};
    std::string message;
};

struct MavgLodSelectionResult {
    std::vector<MavgLodSelectedCluster> selected_clusters;
    std::vector<MavgLodPageRequest> page_requests;
    std::vector<MavgLodSelectionDiagnostic> diagnostics;
    std::size_t fallback_substitution_count{0};
    std::size_t missing_page_count{0};
    bool budget_degraded{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] MavgLodSelectionResult select_mavg_lod_clusters(const MavgClusterGraphDocument& graph,
                                                              const MavgLodViewDesc& view,
                                                              const MavgLodResidentPageSet& resident_pages,
                                                              const MavgLodPreviousSelection& previous = {});

} // namespace mirakana
```

`engine/runtime/include/mirakana/runtime/mavg_lod_residency.hpp` bridges existing resident catalog state into selector input:

```cpp
namespace mirakana::runtime {

struct RuntimeMavgLodResidencyDesc {
    AssetId graph_asset;
    const MavgClusterGraphDocument* graph{nullptr};
    const RuntimeResourceCatalogV2* catalog{nullptr};
};

struct RuntimeMavgLodResidencyResult {
    MavgLodResidentPageSet resident_pages;
    std::vector<MavgLodPageRequest> reviewed_page_requests;
    std::vector<std::string> diagnostics;
    bool invoked_file_io{false};
    bool mutated_mount_set{false};
    bool executed_streaming{false};
    bool touched_renderer_or_rhi_handles{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] RuntimeMavgLodResidencyResult build_runtime_mavg_lod_residency(
    const RuntimeMavgLodResidencyDesc& desc, std::span<const MavgLodPageRequest> page_requests);

} // namespace mirakana::runtime
```

`engine/scene_renderer/include/mirakana/scene_renderer/mavg_scene_lod.hpp` owns conventional submission planning:

```cpp
namespace mirakana {

struct MavgSceneLodMeshBinding {
    AssetId graph_asset;
    MeshGpuBinding mesh_binding;
};

struct MavgSceneLodMaterialBinding {
    AssetId material;
    MaterialGpuBinding material_binding;
};

struct MavgSceneLodSubmitDesc {
    AssetId graph_asset;
    Transform3D transform;
    Color fallback_color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F};
    const MavgSceneLodMeshBinding* mesh_binding{nullptr};
    std::span<const MavgSceneLodMaterialBinding> material_bindings;
};

struct MavgSceneLodSubmitResult {
    std::vector<MeshCommand> mesh_commands;
    std::vector<std::string> diagnostics;
    std::size_t submitted_cluster_count{0};
    std::size_t missing_material_binding_count{0};

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] MavgSceneLodSubmitResult plan_mavg_scene_lod_mesh_commands(
    const MavgLodSelectionResult& selection, const MavgClusterGraphDocument& graph, const MavgSceneLodSubmitDesc& desc);

} // namespace mirakana
```

`engine/renderer/include/mirakana/renderer/renderer.hpp` gains a conventional indexed draw range used by MAVG and by future non-MAVG submesh draws:

```cpp
struct MeshIndexedDrawRange {
    bool enabled{false};
    std::uint32_t first_index{0};
    std::uint32_t index_count{0};
    std::int32_t vertex_base{0};
    std::uint32_t first_instance{0};
};

struct MeshCommand {
    Transform3D transform;
    Color color;
    AssetId mesh;
    AssetId material;
    Mat4 world_from_node{Mat4::identity()};
    MeshGpuBinding mesh_binding;
    MaterialGpuBinding material_binding;
    std::uint32_t instance_count{1};
    MeshIndexedDrawRange indexed_range;
    bool gpu_skinning{false};
    SkinnedMeshGpuBinding skinned_mesh;
    bool gpu_morphing{false};
    MorphMeshGpuBinding morph_mesh;
};
```

`engine/rhi/include/mirakana/rhi/rhi.hpp` clean-breaks the conventional draw command list API:

```cpp
virtual void draw_indexed(std::uint32_t index_count, std::uint32_t instance_count, std::uint32_t first_index = 0,
                          std::int32_t vertex_offset = 0, std::uint32_t first_instance = 0) = 0;
```

## Tasks

### Task 1: Expand Graph Tests For Hierarchy, Error, Fallback, And Draw Ranges

**Files:**

- Modify: `tests/unit/mavg_cluster_graph_tests.cpp`
- Modify: `CMakeLists.txt`

- [x] Add failing tests for:
  - valid parent/child hierarchy with one root and two child clusters
  - parent cycle rejection
  - parent geometric error smaller than child rejection
  - missing resident fallback rejection
  - fallback cluster that is not an ancestor rejection
  - invalid zero `index_count` rejection for drawable clusters
  - invalid draw range exceeding graph-local triangle-count/uint32 bounds invariants; payload byte-length rejection is deferred to Task 3/4 when payload rows exist
  - serialization round trip preserves `parent_cluster_index`, `has_parent`, `resident_fallback_cluster_index`, `geometric_error`, `first_index`, `index_count`, and `vertex_base`

- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_cluster_graph_tests
```

Evidence: baseline passed before RED; RED failed because `MavgClusterGraphCluster` fields and hierarchy diagnostics were missing.

### Task 2: Implement Graph Hierarchy/Error/Fallback Contract

**Files:**

- Modify: `engine/assets/include/mirakana/assets/mavg_cluster_graph.hpp`
- Modify: `engine/assets/src/mavg_cluster_graph.cpp`
- Modify: `tests/unit/mavg_cluster_graph_tests.cpp`

- [x] Add the clean-break fields shown in **Public API Contract**.
- [x] Add diagnostics:
  - `missing_root_cluster`
  - `missing_parent_cluster`
  - `parent_cycle`
  - `parent_error_less_than_child`
  - `missing_resident_fallback`
  - `fallback_not_ancestor`
  - `invalid_cluster_draw_range`
  - `invalid_cluster_geometric_error`
- [x] Canonicalize by page, material partition, cluster id, then sorted children.
- [x] Serialize the expanded rows under `GameEngine.MavgClusterGraph.v1`; do not add a compatibility reader for old rows.
- [x] Deserialize only the expanded row grammar.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_cluster_graph_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_mavg_cluster_graph_tests
```

Evidence: `MK_mavg_cluster_graph_tests` passes after implementation. Additional focused checks passed: `tools/check-format.ps1`, `tools/check-public-api-boundaries.ps1`, and `tools/check-tidy.ps1 -Files engine/assets/src/mavg_cluster_graph.cpp,tests/unit/mavg_cluster_graph_tests.cpp -ReuseExistingFileApiReply`.

### Task 3: Expand Cook Tests For Draw-Ready Static Payloads

**Files:**

- Modify: `tests/unit/tools_mavg_cluster_cook_tests.cpp`

- [x] Add failing tests for:
  - deterministic vertex/index payload bytes from a small static mesh
  - deterministic cluster draw ranges
  - root fallback cluster and child fallback ancestry
  - geometric error monotonicity
  - invalid material partition triangle range
  - invalid vertex/index reference
  - produced graph passes expanded `validate_mavg_cluster_graph`

- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_tools_mavg_cluster_cook_tests
```

Evidence: RED failed before `MavgClusterCookVertex`, triangle index rows, and `MavgClusterCookRequest::vertices` existed. The focused green loop passed after implementation with `MK_tools_mavg_cluster_cook_tests`.

### Task 4: Implement Static MAVG Cook Payload v1

**Files:**

- Modify: `engine/tools/include/mirakana/tools/mavg_cluster_cook.hpp`
- Modify: `engine/tools/asset/mavg_cluster_cook.cpp`
- Modify: `tests/unit/tools_mavg_cluster_cook_tests.cpp`

- [x] Add cook input rows:

```cpp
struct MavgClusterCookVertex {
    MavgVec3f position;
    MavgVec3f normal;
    float u{0.0F};
    float v{0.0F};
};

struct MavgClusterCookTriangle {
    std::uint32_t i0{0};
    std::uint32_t i1{0};
    std::uint32_t i2{0};
    MavgBounds3f bounds;
};
```

- [x] Add `std::vector<MavgClusterCookVertex> vertices` to `MavgClusterCookRequest`.
- [x] Emit deterministic payload bytes with little-endian float32 rows and uint32 index rows rebased by the material partition `vertex_base`.
- [x] Assign `first_index`, `index_count`, `vertex_base`, and `vertex_count` per cluster so conventional indexed draws can use partition-local indices over the shared vertex buffer.
- [x] Validate material partition ranges cover every source triangle exactly once, and sort partitions by triangle/material identity so output is independent of input container order.
- [x] Build a simple per-material root/leaf hierarchy: each material root cluster is resident fallback; leaf clusters point at that root until a later simplifier plan replaces this with multi-level hierarchy.
- [x] Compute `geometric_error` conservatively from cluster/root bounds radius so parent error is never smaller than child error.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_tools_mavg_cluster_cook_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_mavg_cluster_graph_tests|MK_tools_mavg_cluster_cook_tests"
```

Evidence: `tools/cmake.ps1 --build --preset dev --target MK_tools_mavg_cluster_cook_tests MK_mavg_cluster_graph_tests MK_tools_tests`, `tools/ctest.ps1 --preset dev --output-on-failure -R "MK_(tools_mavg_cluster_cook|mavg_cluster_graph|tools)_tests"` passed 3/3, `tools/check-tidy.ps1 -Files engine/tools/asset/mavg_cluster_cook.cpp,tests/unit/tools_mavg_cluster_cook_tests.cpp -ReuseExistingFileApiReply` passed 2 files, and full `tools/validate.ps1` passed with 103/103 CTest targets for this main-based static cook payload checkpoint on 2026-06-07.

### Task 5: Add Failing CPU LOD Selector Tests

**Files:**

- Create: `tests/unit/mavg_lod_selection_tests.cpp`
- Modify: `CMakeLists.txt`

- [x] Add `MK_mavg_lod_selection_tests` linked to `MK_renderer`.
- [x] Add tests for:
  - near camera selects child clusters
  - far camera selects root fallback cluster
  - nonresident child page selects resident fallback and emits page request
  - nonresident child with missing fallback reports diagnostic
  - `max_selected_clusters` degrades selection deterministically
  - previous selection plus hysteresis keeps stable output near the threshold
  - invalid view rows return diagnostics and no selected rows
  - invalid graph returns diagnostics and no selected rows
  - mixed-residency child pages collapse to one resident fallback cover without overlapping sibling rows
  - impossible `max_selected_clusters` budgets return an explicit unsatisfied-budget diagnostic instead of over-budget success

- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_lod_selection_tests
```

Expected: fail before `mavg_lod_selection.hpp` exists.

Evidence: RED failed before `mavg_lod_selection.hpp` existed with `fatal error C1083: cannot open include file 'mirakana/renderer/mavg_lod_selection.hpp'`.

### Task 6: Implement CPU LOD Selector

**Files:**

- Modify: `engine/renderer/CMakeLists.txt`
- Create: `engine/renderer/include/mirakana/renderer/mavg_lod_selection.hpp`
- Create: `engine/renderer/src/mavg_lod_selection.cpp`
- Modify: `tests/unit/mavg_lod_selection_tests.cpp`

- [x] Implement `select_mavg_lod_clusters`.
- [x] Use finite view parameters only; reject zero/negative viewport height and target error.
- [x] Estimate screen error as `geometric_error * viewport_height_pixels / max(distance_to_bounds_center, epsilon)`.
- [x] Traverse from roots to children while resident and above target error.
- [x] If a selected fine cluster page is missing, substitute `resident_fallback_cluster_index` and emit a page request for the missing page.
- [x] Sort output rows by `material_partition`, `page_index`, then `cluster_index`.
- [x] Preserve deterministic budget degradation by replacing highest-cost child groups with resident fallback ancestors until `max_selected_clusters` is satisfied, or return an explicit unsatisfied-budget diagnostic when the graph cannot be reduced without dropping root coverage.
- [x] Apply hysteresis only when previous rows are valid and within `hysteresis_pixels`.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_lod_selection_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_mavg_lod_selection_tests
```

Expected: selector tests pass.

Evidence: `tools/cmake.ps1 --build --preset dev --target MK_mavg_lod_selection_tests`, `tools/ctest.ps1 --preset dev --output-on-failure -R "MK_(mavg_cluster_graph|tools_mavg_cluster_cook|mavg_lod_selection)_tests"`, `tools/check-tidy.ps1 -Files engine/renderer/src/mavg_lod_selection.cpp,tests/unit/mavg_lod_selection_tests.cpp -ReuseExistingFileApiReply`, `tools/check-public-api-boundaries.ps1`, `tools/check-format.ps1`, `tools/check-json-contracts.ps1`, `tools/check-agents.ps1`, `tools/check-ai-integration.ps1`, and full `tools/validate.ps1` passed with 104/104 CTest targets for the CPU selector checkpoint on 2026-06-07.

### Task 7: Add Runtime Resident Page Bridge Tests

**Files:**

- Create: `tests/unit/runtime_mavg_lod_residency_tests.cpp`
- Modify: `CMakeLists.txt`

- [x] Add `MK_runtime_mavg_lod_residency_tests` linked to `MK_runtime`.
- [x] Add tests for:
  - package catalog rows with `AssetKind::mavg_cluster_graph` plus caller-owned graph rows produce resident page ids
  - missing catalog evidence produces diagnostics
  - valid page requests are preserved as reviewed page requests; mismatched graph assets or page ids outside the graph diagnose `invalid-page-request`
  - bridge does not load files, mutate mount sets, execute streaming, or touch renderer/RHI handles

- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_lod_residency_tests
```

Expected: fail before runtime bridge exists.

Evidence: RED confirmed with `tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_lod_residency_tests`; compile failed on missing `mirakana/runtime/mavg_lod_residency.hpp` before production code existed.

### Task 8: Implement Runtime Resident Page Bridge

**Files:**

- Modify: `engine/runtime/CMakeLists.txt`
- Create: `engine/runtime/include/mirakana/runtime/mavg_lod_residency.hpp`
- Create: `engine/runtime/src/mavg_lod_residency.cpp`
- Modify: `tests/unit/runtime_mavg_lod_residency_tests.cpp`

- [x] Implement `build_runtime_mavg_lod_residency`.
- [x] Treat runtime catalog rows as already-reviewed resident evidence only.
- [x] Use a caller-owned `MavgClusterGraphDocument` observer because `RuntimeResourceCatalogV2` has resident resource rows, not page payload contents.
- [x] Keep `MavgLodResidentPageSet` and `MavgLodPageRequest` in `MK_assets` shared graph headers so `MK_runtime` does not depend on `MK_renderer`.
- [x] Do not read package files or mutate `RuntimeResidentPackageMountSetV2`.
- [x] Keep validated page requests as explicit review rows for a later package streaming child plan.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_lod_residency_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_mavg_lod_residency_tests
```

Expected: runtime residency bridge tests pass.

Evidence: `tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_lod_residency_tests`, `tools/ctest.ps1 --preset dev --output-on-failure -R "MK_(mavg_cluster_graph|tools_mavg_cluster_cook|mavg_lod_selection|runtime_mavg_lod_residency)_tests"`, `tools/check-tidy.ps1 -Files engine/runtime/src/mavg_lod_residency.cpp,tests/unit/runtime_mavg_lod_residency_tests.cpp,engine/renderer/src/mavg_lod_selection.cpp -ReuseExistingFileApiReply`, and `tools/check-public-api-boundaries.ps1` passed for the runtime residency bridge checkpoint.

### Task 9: Add Conventional Indexed Draw Range Tests

**Files:**

- Modify: `tests/unit/rhi_tests.cpp`
- Modify: `tests/unit/d3d12_rhi_tests.cpp`
- Modify: `tests/unit/backend_scaffold_tests.cpp`
- Modify: `tests/unit/renderer_rhi_tests.cpp`

- [ ] Add failing tests for:
  - Null RHI records `first_index`, `vertex_offset`, and `first_instance`
  - D3D12 backend uses `DrawIndexedInstanced(index_count, instance_count, first_index, vertex_offset, first_instance)`
  - Vulkan backend uses `vkCmdDrawIndexed(index_count, instance_count, first_index, vertex_offset, first_instance)`
  - `RhiFrameRenderer`, `RhiPostprocessFrameRenderer`, and `RhiDirectionalShadowSmokeFrameRenderer` use `MeshCommand::indexed_range` when enabled
  - default disabled range preserves existing full-mesh draw behavior

- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_rhi_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_backend_scaffold_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests
```

Expected: fail before range-aware draw APIs exist.

### Task 10: Implement Conventional Indexed Draw Range

**Files:**

- Modify: `engine/rhi/include/mirakana/rhi/rhi.hpp`
- Modify: `engine/rhi/src/null_rhi.cpp`
- Modify: `engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp`
- Modify: `engine/rhi/d3d12/src/d3d12_backend.cpp`
- Modify: `engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_backend.hpp`
- Modify: `engine/rhi/vulkan/src/vulkan_backend.cpp`
- Modify: `engine/renderer/include/mirakana/renderer/renderer.hpp`
- Modify: `engine/renderer/src/rhi_frame_renderer.cpp`
- Modify: `engine/renderer/src/rhi_postprocess_frame_renderer.cpp`
- Modify: `engine/renderer/src/rhi_directional_shadow_smoke_frame_renderer.cpp`
- Modify: `engine/renderer/src/null_renderer.cpp`

- [ ] Add `MeshIndexedDrawRange` and `MeshCommand::indexed_range`.
- [ ] Change `rhi::IRhiCommandList::draw_indexed` to accept `first_index`, `vertex_offset`, and `first_instance`.
- [ ] Update Null RHI, D3D12, and Vulkan implementations to pass the range to the backend-native indexed draw call.
- [ ] In renderer implementations, choose:
  - range disabled: existing binding index count and zero offsets
  - range enabled: range `index_count`, `first_index`, `vertex_base`, and `first_instance`
- [ ] Reject enabled ranges with zero `index_count`.
- [ ] Do not add indirect draw, command signatures, mesh shader dispatch, or public native handles.
- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_rhi_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_backend_scaffold_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_rhi_tests|MK_backend_scaffold_tests|MK_renderer_tests"
```

Expected: RHI and renderer range tests pass.

### Task 11: Add Scene Renderer Conventional Submission Tests

**Files:**

- Create: `tests/unit/scene_renderer_mavg_lod_tests.cpp`
- Modify: `CMakeLists.txt`

- [ ] Add `MK_scene_renderer_mavg_lod_tests` linked to `MK_scene_renderer`.
- [ ] Add tests for:
  - selected clusters produce `MeshCommand` rows using existing `MeshGpuBinding`
  - selected clusters enable `MeshIndexedDrawRange` with cluster `first_index`, `index_count`, and `vertex_base`
  - missing material binding uses fallback color and diagnostic
  - fallback substitution rows remain marked in diagnostics/counters
  - no native/RHI handles are exposed beyond existing opaque bindings
  - `NullRenderer` can receive planned commands through existing `draw_mesh`

- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_scene_renderer_mavg_lod_tests
```

Expected: fail before scene-renderer MAVG LOD submission exists.

### Task 12: Implement Conventional Scene Submission

**Files:**

- Modify: `engine/scene_renderer/CMakeLists.txt`
- Create: `engine/scene_renderer/include/mirakana/scene_renderer/mavg_scene_lod.hpp`
- Create: `engine/scene_renderer/src/mavg_scene_lod.cpp`
- Modify: `tests/unit/scene_renderer_mavg_lod_tests.cpp`

- [ ] Implement `plan_mavg_scene_lod_mesh_commands`.
- [ ] Reuse existing `MeshCommand`, `MeshGpuBinding`, and `MaterialGpuBinding`.
- [ ] Set `MeshCommand::mesh` to the MAVG graph asset and `MeshCommand::material` from the selected material partition.
- [ ] Set `MeshCommand::indexed_range` from cluster `first_index`, `index_count`, and `vertex_base`.
- [ ] Leave `MeshGpuBinding` ownership and buffer handles unchanged.
- [ ] Keep renderer submission conventional: no `ExecuteIndirect`, no mesh shader dispatch, no backend-specific command signatures.
- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_scene_renderer_mavg_lod_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_mavg_lod_selection_tests|MK_runtime_mavg_lod_residency_tests|MK_scene_renderer_mavg_lod_tests"
```

Expected: selector, residency bridge, and scene submission tests pass.

### Task 13: Sync Docs, Manifest, And Static Contracts

**Files:**

- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Compose: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration-010-agent-baseline.ps1`
- Modify: `tools/check-ai-integration-020-engine-manifest.ps1`
- Modify: `tools/check-json-contracts-030-tooling-contracts.ps1`
- Modify: `tools/check-json-contracts-040-agent-surfaces.ps1`

- [ ] Record implemented surfaces:
  - `mavg_lod_selection.hpp`
  - `MavgLodViewDesc`
  - `MavgLodResidentPageSet`
  - `select_mavg_lod_clusters`
  - `mavg_lod_residency.hpp`
  - `build_runtime_mavg_lod_residency`
  - `mavg_scene_lod.hpp`
  - `plan_mavg_scene_lod_mesh_commands`
  - `MeshIndexedDrawRange`
  - range-aware `rhi::IRhiCommandList::draw_indexed`
- [ ] Keep non-claims:
  - GPU culling
  - indirect draw execution
  - mesh shaders
  - package streaming execution
  - deformation
  - ray tracing
  - Metal readiness
  - Nanite compatibility/equivalence/superiority
  - broad CPU/GPU/memory optimization
- [ ] Compose manifest:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
```

- [ ] Run drift checks:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
```

Expected: all checks pass.

### Task 14: Slice Validation And Publication

**Files:**

- Validate all touched C++ code, tests, docs, manifest fragments, composed manifest, and static checks.

- [ ] Run focused C++ validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_cluster_graph_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_tools_mavg_cluster_cook_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_rhi_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_backend_scaffold_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_lod_selection_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_lod_residency_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_scene_renderer_mavg_lod_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_mavg_cluster_graph_tests|MK_tools_mavg_cluster_cook_tests|MK_rhi_tests|MK_backend_scaffold_tests|MK_renderer_tests|MK_mavg_lod_selection_tests|MK_runtime_mavg_lod_residency_tests|MK_scene_renderer_mavg_lod_tests"
```

- [ ] Run full validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
git diff --check
```

- [ ] Run publication preflight:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1
```

Expected: focused tests, full validation, whitespace check, and publication preflight pass or record a concrete host/tool blocker.

## Done When

- The graph contract validates hierarchy, geometric error, draw ranges, and resident fallback ancestry.
- The cook planner emits deterministic static draw payloads and expanded graph rows.
- CPU LOD selection is deterministic, budget-aware, hysteresis-aware, and never creates holes when a resident fallback ancestor exists.
- Runtime resident page bridge converts existing resident package/catalog evidence into selector input without executing streaming.
- RHI and renderer conventional draw paths support explicit indexed draw ranges.
- Scene renderer planning can produce range-aware conventional `MeshCommand` rows for selected clusters using existing opaque GPU bindings.
- Docs, plans, registry, manifest fragments, composed manifest, and static checks describe exactly the implemented static conventional LOD scope.
- Full `tools/validate.ps1` passes or records a concrete host/tool blocker.

## Non-Claims

- No GPU culling.
- No D3D12 `ExecuteIndirect` or Vulkan indirect draw execution.
- No mesh/task/amplification shader readiness.
- No background package streaming execution.
- No GPU memory residency enforcement beyond existing value/evidence rows.
- No skinned, morph, displacement, destruction, or dynamic cluster support.
- No ray tracing payload or BLAS/TLAS integration.
- No Metal readiness from Windows validation.
- No Nanite compatibility, Nanite-equivalence, Nanite-superiority, or benchmark-exceeds claim.

## Follow-Up Child Plans

After this milestone lands:

- `mavg-gpu-culling-indirect-v1`: compute culling, indirect command buffers, D3D12/Vulkan synchronization evidence.
- `mavg-package-streaming-residency-v1`: background/async page request execution, reviewed eviction policy, GPU memory pressure integration.
- `mavg-mesh-shader-backends-v1`: D3D12 mesh/amplification shader path, Vulkan `VK_EXT_mesh_shader` path, strict feature gates, fallback preservation.
- `mavg-deformable-clusters-v1`: rigid, skinned, morph, and dynamic update tiers.
- `mavg-ray-tracing-consistency-v1`: raster/RT payload consistency and backend-local acceleration structure evidence.
- `mavg-benchmark-claim-ladder-v1`: first-party scenes, conventional path comparison, host matrix, trace evidence, and claim wording review.
