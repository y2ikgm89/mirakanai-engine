# 2026-06-05 MAVG Runtime LOD Milestone v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` or focused inline TDD execution to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `mavg-runtime-lod-milestone-v1`

**Status:** Parent milestone for the active `mavg-rhi-indirect-draw-v1` child.

**Execution State:** Parent milestone in the stacked MAVG implementation branch after explicit LoD supersession. `mavg-asset-graph-v1` remains the prerequisite foundation branch; `mavg-gpu-culling-indirect-v1`, `mavg-rhi-indirect-draw-v1`, `mavg-d3d12-indexed-indirect-draw-execution-v1`, `mavg-d3d12-indexed-indirect-count-buffer-execution-v1`, and `mavg-d3d12-compute-generated-indirect-execution-v1` are completed stacked children. `mavg-d3d12-gpu-culling-execution-v1` is the active child for the selected D3D12 WARP-backed MAVG visibility compaction and indexed indirect execution proof. This milestone remains the conventional static LoD evidence record and must not claim broad GPU culling frameworks, Vulkan/Metal backend indirect execution, or Nanite equivalence/superiority.

**Goal:** Implement the first visible static MAVG LOD path: deterministic hierarchy/error/fallback graph rows, deterministic CPU LOD selection, package-resident page awareness, conventional indexed draw-range support, and conventional renderer submission without GPU culling, mesh shaders, streaming IO execution, deformation, ray tracing, or Nanite-equivalence claims.

**Architecture:** Close the current asset-graph gaps first, then add a value-only `MK_renderer` CPU selector, then bridge selected rows into `MK_runtime` resident package/catalog evidence, and finally submit selected clusters through the existing conventional indexed mesh path in `MK_scene_renderer`. GPU-driven culling, indirect draw execution, mesh shaders, background streaming, deformation tiers, ray tracing, and benchmark superiority stay separate future child plans with their own backend/host evidence.

**Tech Stack:** C++23, `MK_assets`, `MK_tools`, `MK_renderer`, `MK_runtime`, `MK_runtime_rhi`, `MK_scene_renderer`, existing `MK_rhi` upload/frame-graph paths, CMake/CTest, PowerShell validation scripts, Context7-checked CMake/Vulkan docs, Microsoft Direct3D 12 docs, Khronos Vulkan docs, and Apple Metal docs.

---

## Current Project Audit

`mavg-asset-graph-v1` has implemented the first data-only foundation:

- `engine/assets/include/mirakana/assets/mavg_cluster_graph.hpp` exposes `MavgClusterGraphDocument`, `MavgClusterGraphCluster::lod_level`, page rows, material partitions, child ids, validation, canonicalization, text serialization, and text deserialization.
- `engine/tools/include/mirakana/tools/mavg_cluster_cook.hpp` exposes `MavgClusterCookRequest`, `plan_mavg_cluster_graph_cook_package`, and `apply_mavg_cluster_graph_cook_package`.
- The cook request now carries static `MavgClusterCookVertex` rows plus indexed `MavgClusterCookTriangle` rows and emits deterministic `GameEngine.MavgClusterPayload.v1` `vertex.data_hex` / `index.data_hex` rows.
- This milestone's implementation checkpoints now add graph parent ids, geometric error, resident fallback ancestors, cluster draw ranges, draw-ready static cook payload rows, a deterministic value-only `MK_renderer` CPU selector, an `MK_runtime` resident-page evidence bridge, `MK_runtime` caller-reviewed page streaming planning and reviewed visible/fallback-ancestor eviction protection, range-aware conventional indexed draw execution, `MK_scene_renderer` conventional `MeshCommand` planning, and `MK_runtime_rhi` conventional package-visible MAVG mesh binding upload evidence. Background/autonomous page package streaming execution remains future work.
- `MK_runtime` already has resident package mount sets, resident catalog caches, byte/record budget checks, selected safe-point package streaming, and reviewed eviction-assisted commit helpers.
- `MK_renderer` already has `MeshCommand`, `MeshGpuBinding`, `RendererStats`, `NullRenderer`, frame graph/RHI policies, GPU memory policy rows, and renderer quality evidence surfaces.
- `MeshCommand` and `rhi::IRhiCommandList::draw_indexed` now expose selected index ranges, so a visible conventional MAVG LOD path can submit selected clusters through the existing indexed mesh path.
- `MK_scene_renderer` converts `SceneRenderPacket` meshes into `MeshCommand` rows and submits them through `IRenderer::draw_mesh`; `mavg_scene_lod.hpp` now plans selected MAVG cluster rows into conventional range-aware `MeshCommand` rows without extending `SceneRenderPacket`.
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
    hysteresis_reused_previous,
};

struct MavgLodViewDesc {
    Mat4 clip_from_world{Mat4::identity()};
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
- [x] Emit deterministic payload bytes with little-endian float32 rows and uint32 index rows.
- [x] Assign `first_index`, `index_count`, and `vertex_base` per cluster.
- [x] Build a simple per-material root/leaf hierarchy: each material root cluster is resident fallback; leaf clusters point at that root until a later simplifier plan replaces this with multi-level hierarchy.
- [x] Compute `geometric_error` conservatively from cluster/root bounds radius so parent error is never smaller than child error.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_tools_mavg_cluster_cook_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_mavg_cluster_graph_tests|MK_tools_mavg_cluster_cook_tests"
```

Evidence: `tools/cmake.ps1 --build --preset dev --target MK_tools_mavg_cluster_cook_tests`, `tools/ctest.ps1 --preset dev --output-on-failure -R "MK_(mavg_cluster_graph|tools_mavg_cluster_cook)_tests"`, `tools/check-tidy.ps1 -Files engine/tools/asset/mavg_cluster_cook.cpp,tests/unit/tools_mavg_cluster_cook_tests.cpp -ReuseExistingFileApiReply`, `tools/check-public-api-boundaries.ps1`, `tools/format.ps1`, and `git diff --check` passed for the static cook payload checkpoint.

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
- [x] Preserve deterministic budget degradation by replacing highest-cost child groups with fallback ancestors until `max_selected_clusters` is satisfied.
- [x] Apply hysteresis only when previous rows are valid and within `hysteresis_pixels`.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_lod_selection_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_mavg_lod_selection_tests
```

Expected: selector tests pass.

Evidence: `tools/cmake.ps1 --build --preset dev --target MK_mavg_lod_selection_tests`, `tools/ctest.ps1 --preset dev --output-on-failure -R "MK_(mavg_cluster_graph|tools_mavg_cluster_cook|mavg_lod_selection)_tests"`, `tools/check-tidy.ps1 -Files engine/renderer/src/mavg_lod_selection.cpp,tests/unit/mavg_lod_selection_tests.cpp -ReuseExistingFileApiReply`, `tools/check-public-api-boundaries.ps1`, `tools/check-format.ps1`, `tools/check-json-contracts.ps1`, `tools/check-agents.ps1`, and `tools/check-ai-integration.ps1` passed for the CPU selector checkpoint.

### Task 7: Add Runtime Resident Page Bridge Tests

**Files:**

- Create: `tests/unit/runtime_mavg_lod_residency_tests.cpp`
- Modify: `CMakeLists.txt`

- [x] Add `MK_runtime_mavg_lod_residency_tests` linked to `MK_runtime`.
- [x] Add tests for:
  - package catalog rows with `AssetKind::mavg_cluster_graph` plus caller-owned graph rows produce resident page ids
  - missing catalog evidence produces diagnostics
  - page requests are preserved as reviewed page requests
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
- [x] Keep page requests as explicit review rows for a later package streaming child plan.
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

- [x] Add failing tests for:
  - Null RHI records `first_index`, `vertex_offset`, and `first_instance`
  - D3D12 backend uses `DrawIndexedInstanced(index_count, instance_count, first_index, vertex_offset, first_instance)`
  - Vulkan backend uses `vkCmdDrawIndexed(index_count, instance_count, first_index, vertex_offset, first_instance)`
  - `RhiFrameRenderer`, `RhiPostprocessFrameRenderer`, and `RhiDirectionalShadowSmokeFrameRenderer` use `MeshCommand::indexed_range` when enabled
  - default disabled range preserves existing full-mesh draw behavior

- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_rhi_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_backend_scaffold_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests
```

Expected: fail before range-aware draw APIs exist.

Evidence: RED confirmed on 2026-06-05 with expected compile failures before implementation: `tools/cmake.ps1 --build --preset dev --target MK_rhi_tests` failed because `IRhiCommandList::draw_indexed` did not accept `first_index`, `vertex_offset`, and `first_instance`, and `RhiStats` did not expose last indexed draw range rows; `tools/cmake.ps1 --build --preset dev --target MK_backend_scaffold_tests` failed for the same missing RHI range API/stat rows; `tools/cmake.ps1 --build --preset dev --target MK_renderer_tests` failed because `MeshIndexedDrawRange` and `MeshCommand::indexed_range` did not exist.

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

- [x] Add `MeshIndexedDrawRange` and `MeshCommand::indexed_range`.
- [x] Change `rhi::IRhiCommandList::draw_indexed` to accept `first_index`, `vertex_offset`, and `first_instance`.
- [x] Update Null RHI, D3D12, and Vulkan implementations to pass the range to the backend-native indexed draw call.
- [x] In renderer implementations, choose:
  - range disabled: existing binding index count and zero offsets
  - range enabled: range `index_count`, `first_index`, `vertex_base`, and `first_instance`
- [x] Reject enabled ranges with zero `index_count`.
- [x] Do not add indirect draw, command signatures, mesh shader dispatch, or public native handles.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_rhi_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_backend_scaffold_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_rhi_tests|MK_backend_scaffold_tests|MK_renderer_tests"
```

Expected: RHI and renderer range tests pass.

Evidence: GREEN confirmed on 2026-06-05 after implementation with `tools/cmake.ps1 --build --preset dev --target MK_rhi_tests`, `tools/cmake.ps1 --build --preset dev --target MK_backend_scaffold_tests`, `tools/cmake.ps1 --build --preset dev --target MK_renderer_tests`, `tools/ctest.ps1 --preset dev --output-on-failure -R "MK_rhi_tests|MK_backend_scaffold_tests|MK_renderer_tests"`, `tools/cmake.ps1 --build --preset dev --target MK_d3d12_rhi_tests`, and `tools/ctest.ps1 --preset dev --output-on-failure -R MK_d3d12_rhi_tests`. Official API context was re-checked against Microsoft Learn `ID3D12GraphicsCommandList::DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation)` and Context7 `/khronosgroup/vulkan-docs` `vkCmdDrawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance)`.

### Task 11: Add Scene Renderer Conventional Submission Tests

**Files:**

- Create: `tests/unit/scene_renderer_mavg_lod_tests.cpp`
- Modify: `CMakeLists.txt`

- [x] Add `MK_scene_renderer_mavg_lod_tests` linked to `MK_scene_renderer`.
- [x] Add tests for:
  - selected clusters produce `MeshCommand` rows using existing `MeshGpuBinding`
  - selected clusters enable `MeshIndexedDrawRange` with cluster `first_index`, `index_count`, and `vertex_base`
  - missing material binding uses fallback color and diagnostic
  - fallback substitution rows remain marked in diagnostics/counters
  - no native/RHI handles are exposed beyond existing opaque bindings
  - `NullRenderer` can receive planned commands through existing `draw_mesh`

- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_scene_renderer_mavg_lod_tests
```

Expected: fail before scene-renderer MAVG LOD submission exists.

Evidence: RED confirmed on 2026-06-05 with `tools/cmake.ps1 --build --preset dev --target MK_scene_renderer_mavg_lod_tests`; after configuring the new worktree, the build failed because `mirakana/scene_renderer/mavg_scene_lod.hpp` did not exist.

### Task 12: Implement Conventional Scene Submission

**Files:**

- Modify: `engine/scene_renderer/CMakeLists.txt`
- Create: `engine/scene_renderer/include/mirakana/scene_renderer/mavg_scene_lod.hpp`
- Create: `engine/scene_renderer/src/mavg_scene_lod.cpp`
- Modify: `tests/unit/scene_renderer_mavg_lod_tests.cpp`

- [x] Implement `plan_mavg_scene_lod_mesh_commands`.
- [x] Reuse existing `MeshCommand`, `MeshGpuBinding`, and `MaterialGpuBinding`.
- [x] Set `MeshCommand::mesh` to the MAVG graph asset and `MeshCommand::material` from the selected material partition.
- [x] Set `MeshCommand::indexed_range` from cluster `first_index`, `index_count`, and `vertex_base`.
- [x] Leave `MeshGpuBinding` ownership and buffer handles unchanged.
- [x] Keep renderer submission conventional: no `ExecuteIndirect`, no mesh shader dispatch, no backend-specific command signatures.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_scene_renderer_mavg_lod_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_mavg_lod_selection_tests|MK_runtime_mavg_lod_residency_tests|MK_scene_renderer_mavg_lod_tests"
```

Expected: selector, residency bridge, and scene submission tests pass.

Evidence: GREEN confirmed on 2026-06-05 with `tools/cmake.ps1 --build --preset dev --target MK_scene_renderer_mavg_lod_tests`, `tools/cmake.ps1 --build --preset dev --target MK_scene_renderer_mavg_lod_tests MK_scene_renderer_tests MK_mavg_lod_selection_tests MK_runtime_mavg_lod_residency_tests`, and `tools/ctest.ps1 --preset dev --output-on-failure -R "MK_mavg_lod_selection_tests|MK_runtime_mavg_lod_residency_tests|MK_scene_renderer_mavg_lod_tests|MK_scene_renderer_tests"`. The first CTest attempt in the fresh worktree failed only because related test executables had not been built yet; after building them, all 4 selected tests passed.

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

- [x] Record implemented surfaces:
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
- [x] Keep non-claims:
  - GPU culling
  - indirect draw execution
  - mesh shaders
  - package streaming execution
  - deformation
  - ray tracing
  - Metal readiness
  - Nanite compatibility/equivalence/superiority
  - broad CPU/GPU/memory optimization
- [x] Compose manifest:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
```

- [x] Run drift checks:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
```

Expected: all checks pass.

Evidence: Docs, plan registry, `engine/agent/manifest.fragments/004-modules.json`, `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`, and static MAVG needles were updated for `mavg_scene_lod.hpp`, `MavgSceneLodSubmitDesc`, `MavgSceneLodSubmitResult`, `plan_mavg_scene_lod_mesh_commands`, and `MK_scene_renderer_mavg_lod_tests`; `tools/compose-agent-manifest.ps1 -Write`, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, and `tools/check-agents.ps1` passed on 2026-06-05.

### Task 14: Slice Validation And Publication

**Files:**

- Validate all touched C++ code, tests, docs, manifest fragments, composed manifest, and static checks.

- [x] Run focused C++ validation:

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

Evidence: `tools/cmake.ps1 --preset dev` configured the fresh worktree after the RED include failure; `tools/cmake.ps1 --build --preset dev --target MK_scene_renderer_mavg_lod_tests` passed after implementation; `tools/cmake.ps1 --build --preset dev --target MK_mavg_lod_selection_tests MK_runtime_mavg_lod_residency_tests MK_scene_renderer_tests MK_scene_renderer_mavg_lod_tests` passed; and `tools/ctest.ps1 --preset dev --output-on-failure -R "MK_mavg_lod_selection_tests|MK_runtime_mavg_lod_residency_tests|MK_scene_renderer_mavg_lod_tests|MK_scene_renderer_tests"` passed 4/4 on 2026-06-05. Broader graph, cook, RHI, backend, renderer, and runtime tests were then covered by full validation.

- [x] Run full validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
git diff --check
```

Evidence: `tools/format.ps1`, `tools/check-public-api-boundaries.ps1`, `tools/check-json-contracts.ps1`, `tools/check-tidy.ps1 -Files engine/scene_renderer/src/mavg_scene_lod.cpp,tests/unit/scene_renderer_mavg_lod_tests.cpp -ReuseExistingFileApiReply`, `tools/check-ai-integration.ps1`, `tools/check-agents.ps1`, `tools/check-format.ps1`, and `git diff --check` passed before the full gate. `tools/validate.ps1` passed on 2026-06-05 with static checks ok, build ok, and 104/104 CTest tests passed, including `MK_scene_renderer_mavg_lod_tests`; Windows Metal shader tools and Apple packaging remained diagnostic host gates, not failures.

- [x] Run publication preflight:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1
```

Evidence: `tools/check-publication-preflight.ps1 -Branch codex/mavg-scene-lod-submission-v1` passed on 2026-06-05 with GitHub network reachability, readable `gh` config, and `gh-auth=ok`; the remote head was missing as expected before the first push of this candidate branch.

Expected: focused tests, full validation, whitespace check, and publication preflight pass or record a concrete host/tool blocker.

### Task 15: Add Conventional Runtime Upload Tests

**Files:**

- Modify: `CMakeLists.txt`
- Create: `tests/unit/runtime_rhi_mavg_conventional_upload_tests.cpp`

- [x] Add `MK_runtime_rhi_mavg_conventional_upload_tests` linked to `MK_runtime_rhi` and `MK_scene_renderer`.
- [x] Prove committed package/catalog state plus a `RuntimeMeshPayload` can publish a package-visible MAVG conventional `MeshGpuBinding`.
- [x] Prove the returned binding can feed `plan_mavg_scene_lod_mesh_commands`.
- [x] Prove fail-closed pre-upload diagnostics for non-committed streaming, wrong catalog kind, handle mismatch, and graph draw range overflow.
- [x] Run RED:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_conventional_upload_tests
```

Evidence: RED confirmed on 2026-06-05 after adding the test target; `tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_conventional_upload_tests` failed with MSVC C1083 because `mirakana/runtime_rhi/mavg_conventional_upload.hpp` did not exist.

### Task 16: Implement Conventional Runtime Upload

**Files:**

- Modify: `engine/runtime_rhi/CMakeLists.txt`
- Create: `engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_conventional_upload.hpp`
- Create: `engine/runtime_rhi/src/mavg_conventional_upload.cpp`
- Modify: `tests/unit/runtime_rhi_mavg_conventional_upload_tests.cpp`

- [x] Add `RuntimeMavgConventionalMeshUploadDesc`, `RuntimeMavgConventionalMeshBinding`, `RuntimeMavgConventionalMeshUploadDiagnostic`, and `RuntimeMavgConventionalMeshUploadResult`.
- [x] Add `upload_runtime_mavg_conventional_mesh_binding`.
- [x] Validate committed streaming evidence, live `AssetKind::mavg_cluster_graph` catalog rows, graph/payload asset and handle identity, graph validation, and cluster draw ranges before upload side effects.
- [x] Reuse `upload_runtime_mesh`, `wait_for_runtime_uploads_on_queue`, and `make_runtime_mesh_gpu_binding`.
- [x] Return package-visible counters, uploaded bytes, submitted fences, graph/payload counts, and explicit false flags for GPU culling, indirect draw, mesh shader, and native handle claims.
- [x] Run GREEN:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_conventional_upload_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_rhi_mavg_conventional_upload_tests
```

Evidence: GREEN confirmed on 2026-06-05 with `tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_conventional_upload_tests` and `tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_rhi_mavg_conventional_upload_tests`; the selected test target passed 1/1.

### Task 17: Sync Docs, Manifest, And Static Contracts For Runtime Upload Evidence

**Files:**

- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Compose: `engine/agent/manifest.json`
- Add: `tools/check-ai-integration-104-mavg-runtime-lod.ps1`

- [x] Record implemented runtime RHI MAVG conventional upload surfaces:
  - `mavg_conventional_upload.hpp`
  - `RuntimeMavgConventionalMeshUploadDesc`
  - `RuntimeMavgConventionalMeshUploadResult`
  - `upload_runtime_mavg_conventional_mesh_binding`
  - `MK_runtime_rhi_mavg_conventional_upload_tests`
- [x] Keep non-claims:
  - GPU culling
  - indirect draw execution
  - mesh shaders
  - background/page package streaming execution
  - deformation
  - ray tracing
  - Metal readiness
  - Nanite compatibility/equivalence/superiority
  - broad CPU/GPU/memory optimization
- [x] Compose manifest and run drift checks.

Expected: docs, manifest fragments, composed manifest, and static checks describe exactly the implemented conventional runtime upload/package-visible evidence scope.

Evidence: docs, `engine/agent/manifest.fragments/004-modules.json`, `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`, composed `engine/agent/manifest.json`, and `tools/check-ai-integration-104-mavg-runtime-lod.ps1` were synchronized on 2026-06-05. `tools/compose-agent-manifest.ps1 -Write`, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, and `tools/check-agents.ps1` passed.

### Task 18: Conventional Runtime Upload Slice Validation And Publication

**Files:**

- Validate all touched C++ code, tests, docs, manifest fragments, composed manifest, and static checks.

- [x] Run focused C++ validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_conventional_upload_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_scene_renderer_mavg_lod_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_rhi_mavg_conventional_upload_tests|MK_runtime_rhi_tests|MK_scene_renderer_mavg_lod_tests"
```

- [x] Run static and full validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime_rhi/src/mavg_conventional_upload.cpp,tests/unit/runtime_rhi_mavg_conventional_upload_tests.cpp -ReuseExistingFileApiReply
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
git diff --check
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

- [x] Run publication preflight:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1 -Branch codex/mavg-conventional-runtime-package-evidence-v1
```

Expected: focused tests, full validation, whitespace check, and publication preflight pass or record a concrete host/tool blocker.

Evidence: focused validation passed on 2026-06-05 with `tools/check-toolchain.ps1`, targeted builds for `MK_runtime_rhi_mavg_conventional_upload_tests`, `MK_runtime_rhi_tests`, and `MK_scene_renderer_mavg_lod_tests`, and targeted CTest for those three tests. Static validation passed with `tools/format.ps1`, `tools/check-public-api-boundaries.ps1`, `tools/check-json-contracts.ps1`, targeted `tools/check-tidy.ps1 -Files engine/runtime_rhi/src/mavg_conventional_upload.cpp,tests/unit/runtime_rhi_mavg_conventional_upload_tests.cpp -ReuseExistingFileApiReply`, `tools/check-ai-integration.ps1`, `tools/check-agents.ps1`, `tools/check-format.ps1`, and `git diff --check`. Full `tools/validate.ps1` passed after docs/manifest/static evidence settled with `validate: ok` and CTest `105/105` passing. Publication preflight passed with `tools/check-publication-preflight.ps1 -Branch codex/mavg-conventional-runtime-package-evidence-v1`.

### Task 19: Add Failing MAVG Page Streaming Planner And Safe-Point Drain Tests

**Files:**

- Add: `tests/unit/runtime_mavg_page_streaming_tests.cpp`
- Modify: `CMakeLists.txt`

- [x] Add `MK_runtime_mavg_page_streaming_tests` linked to `MK_runtime`.
- [x] Add planner tests for:
  - resident page skip
  - duplicate page request coalescing by highest priority
  - deterministic priority/page ordering
  - deterministic `max_queued_pages` degradation
  - invalid graph/request rows with zero file IO, mount mutation, streaming execution, and renderer/RHI handles
  - missing page package candidate diagnostics
- [x] Add safe-point drain tests for:
  - one queued row delegates to reviewed resident package candidate mount and refreshes the resident catalog
  - invalid mount id rejects before mount/cache mutation and before candidate load
- [x] Run RED:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_page_streaming_tests
```

Evidence: RED failed before `mavg_page_streaming.hpp` existed with `fatal error C1083: cannot open include file 'mirakana/runtime/mavg_page_streaming.hpp'`.

### Task 20: Implement MAVG Page Streaming Planner And One-Row Safe-Point Drain

**Files:**

- Modify: `engine/runtime/CMakeLists.txt`
- Add: `engine/runtime/include/mirakana/runtime/mavg_page_streaming.hpp`
- Add: `engine/runtime/src/mavg_page_streaming.cpp`
- Modify: `tests/unit/runtime_mavg_page_streaming_tests.cpp`

- [x] Add `RuntimeMavgPageStreamingCandidateRow`, `RuntimeMavgPageStreamingPlanDesc`, `RuntimeMavgPageStreamingPlanRow`, `RuntimeMavgPageStreamingPlanResult`, `RuntimeMavgPageStreamingDrainDesc`, and `RuntimeMavgPageStreamingDrainResult`.
- [x] Add `plan_runtime_mavg_page_streaming_requests` as a pure planner over caller-reviewed `MavgLodPageRequest` rows and package index discovery candidates.
- [x] Add `execute_runtime_mavg_page_streaming_request_safe_point` as a one-row safe-point drain through `commit_runtime_package_candidate_resident_mount_with_reviewed_evictions_v2`.
- [x] Keep non-claims for autonomous background workers, async overlap/performance, automatic eviction policy, partial `.mavgpayload` byte-range page loading/schema, GPU memory pressure enforcement, renderer/RHI handles, GPU culling, indirect draws, mesh shaders, and backend parity.
- [x] Run GREEN:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_page_streaming_tests MK_runtime_mavg_lod_residency_tests MK_runtime_package_streaming_resident_mount_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_mavg_page_streaming_tests|MK_runtime_mavg_lod_residency_tests|MK_runtime_package_streaming_resident_mount_tests"
```

Evidence: GREEN confirmed on 2026-06-05. The first CTest attempt failed only because existing related executables had not been built in the fresh worktree; after building `MK_runtime_mavg_lod_residency_tests` and `MK_runtime_package_streaming_resident_mount_tests`, the selected CTest passed 3/3.

### Task 21: Sync MAVG Page Streaming Docs, Manifest, Static Checks, And Validate

**Files:**

- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Generated: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration-104-mavg-runtime-lod.ps1`

- [x] Describe `mavg_page_streaming.hpp`, `RuntimeMavgPageStreamingPlanResult`, `RuntimeMavgPageStreamingDrainResult`, `plan_runtime_mavg_page_streaming_requests`, and `execute_runtime_mavg_page_streaming_request_safe_point` in current capabilities, roadmap, architecture spec, active plan registry, manifest fragments, composed manifest, and static checks.
- [x] Keep non-claims explicit for autonomous background package streaming workers, async-overlap/performance, automatic eviction policy, partial `.mavgpayload` byte-range page loading/schema, GPU memory pressure integration, GPU culling, indirect draws, mesh shaders, deformation, ray tracing, Metal readiness, Nanite equivalence/superiority, and broad optimization.
- [x] Run docs/manifest/static validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
```

- [x] Run focused C++ validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_page_streaming_tests MK_runtime_mavg_lod_residency_tests MK_runtime_package_streaming_resident_mount_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_mavg_page_streaming_tests|MK_runtime_mavg_lod_residency_tests|MK_runtime_package_streaming_resident_mount_tests"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/mavg_page_streaming.cpp,tests/unit/runtime_mavg_page_streaming_tests.cpp -ReuseExistingFileApiReply
git diff --check
```

- [x] Run full slice validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Evidence: docs/manifest/static validation passed on 2026-06-05 after `tools/compose-agent-manifest.ps1 -Write`, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-agents.ps1`, `tools/format.ps1`, and `tools/check-format.ps1`. Focused C++ validation passed with `tools/check-public-api-boundaries.ps1`, the targeted build for `MK_runtime_mavg_page_streaming_tests`, `MK_runtime_mavg_lod_residency_tests`, and `MK_runtime_package_streaming_resident_mount_tests`, targeted CTest passing 3/3, targeted `tools/check-tidy.ps1 -Files engine/runtime/src/mavg_page_streaming.cpp,tests/unit/runtime_mavg_page_streaming_tests.cpp -ReuseExistingFileApiReply`, and `git diff --check`. Full `tools/validate.ps1` passed with `validate: ok` and CTest `106/106` passing.

### Task 22: Add Failing MAVG Page Streaming Eviction Review Tests

**Files:**

- Modify: `tests/unit/runtime_mavg_page_streaming_tests.cpp`

- [x] Add tests proving selected visible pages and resident fallback ancestors become protected mount ids before eviction planning.
- [x] Add tests proving eviction candidates that target protected selected pages fail through the existing reviewed resident eviction planner.
- [x] Add fail-closed tests for selected graph mismatch, unknown cluster, and missing resident page mount rows before eviction planning.
- [x] Run RED:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_page_streaming_tests
```

Evidence: RED was confirmed on 2026-06-05 after the fresh worktree configure; the target failed because `RuntimeMavgPageStreamingEvictionReviewResult`, `RuntimeMavgResidentPageMountRow`, `RuntimeMavgPageStreamingSelectedClusterRow`, `RuntimeMavgPageStreamingEvictionReviewDesc`, `review_runtime_mavg_page_streaming_evictions`, and the selected/fallback eviction review diagnostics were not yet defined.

### Task 23: Implement MAVG Page Streaming Reviewed Eviction Protection

**Files:**

- Modify: `engine/runtime/include/mirakana/runtime/mavg_page_streaming.hpp`
- Modify: `engine/runtime/src/mavg_page_streaming.cpp`
- Modify: `tests/unit/runtime_mavg_page_streaming_tests.cpp`

- [x] Add `RuntimeMavgResidentPageMountRow`, `RuntimeMavgPageStreamingSelectedClusterRow`, `RuntimeMavgPageStreamingEvictionReviewDesc`, and `RuntimeMavgPageStreamingEvictionReviewResult`.
- [x] Add `review_runtime_mavg_page_streaming_evictions` as a pure review helper that validates graph, resident page mount, selected cluster, and caller-protected mount rows.
- [x] Protect selected cluster pages plus resident fallback ancestor pages before delegating caller-reviewed candidate order to `plan_runtime_resident_package_evictions_v2`.
- [x] Keep non-claims for automatic eviction policy, inferred LRU/frequency policy, autonomous background workers, partial `.mavgpayload` byte-range page loading/schema, GPU memory pressure integration, renderer/RHI handles, file IO, and live mount mutation.
- [x] Run GREEN:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_page_streaming_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "runtime_mavg_page_streaming"
```

Evidence: GREEN was confirmed on 2026-06-05 with `MK_runtime_mavg_page_streaming_tests` build passing and targeted CTest passing 1/1 after implementation.

### Task 24: Sync MAVG Reviewed Eviction Protection Docs, Manifest, Static Checks, And Validate

**Files:**

- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Generated: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration-104-mavg-runtime-lod.ps1`

- [x] Describe `RuntimeMavgPageStreamingEvictionReviewResult`, `RuntimeMavgResidentPageMountRow`, `RuntimeMavgPageStreamingSelectedClusterRow`, and `review_runtime_mavg_page_streaming_evictions` in current capabilities, roadmap, architecture spec, active plan registry, manifest fragments, composed manifest, and static checks.
- [x] Keep non-claims explicit for automatic eviction policy, autonomous background package streaming workers, async-overlap/performance, partial `.mavgpayload` byte-range page loading/schema, GPU memory pressure integration, GPU culling, indirect draws, mesh shaders, deformation, ray tracing, Metal readiness, Nanite equivalence/superiority, and broad optimization.
- [x] Run focused and full validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_page_streaming_tests MK_runtime_mavg_lod_residency_tests MK_runtime_package_streaming_resident_mount_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_mavg_page_streaming_tests|MK_runtime_mavg_lod_residency_tests|MK_runtime_package_streaming_resident_mount_tests"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/mavg_page_streaming.cpp,tests/unit/runtime_mavg_page_streaming_tests.cpp -ReuseExistingFileApiReply
git diff --check
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Evidence: docs/manifest/static validation passed on 2026-06-05 after `tools/compose-agent-manifest.ps1 -Write`, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-agents.ps1`, `tools/format.ps1`, and `tools/check-format.ps1`. Focused C++ validation passed with `tools/check-public-api-boundaries.ps1`, targeted build for `MK_runtime_mavg_page_streaming_tests`, `MK_runtime_mavg_lod_residency_tests`, and `MK_runtime_package_streaming_resident_mount_tests`, targeted CTest passing 3/3, targeted `tools/check-tidy.ps1 -Files engine/runtime/src/mavg_page_streaming.cpp,tests/unit/runtime_mavg_page_streaming_tests.cpp -ReuseExistingFileApiReply`, and `git diff --check`. Full `tools/validate.ps1` passed with `validate: ok` and CTest `106/106` passing.

## Done When

- The graph contract validates hierarchy, geometric error, draw ranges, and resident fallback ancestry.
- The cook planner emits deterministic static draw payloads and expanded graph rows.
- CPU LOD selection is deterministic, budget-aware, hysteresis-aware, and never creates holes when a resident fallback ancestor exists.
- Runtime resident page bridge converts existing resident package/catalog evidence into selector input without executing streaming.
- Runtime page streaming planning converts reviewed selector page requests into deterministic package candidate rows, protects selected visible pages plus resident fallback ancestors before reviewed eviction planning, and can drain one row through reviewed safe-point resident mount execution without autonomous background workers, automatic eviction policy, file IO in review planning, live mount mutation, or renderer/RHI handles.
- RHI and renderer conventional draw paths support explicit indexed draw ranges.
- Scene renderer planning can produce range-aware conventional `MeshCommand` rows for selected clusters using existing opaque GPU bindings.
- Runtime RHI can publish a package-visible conventional MAVG `MeshGpuBinding` from a committed streaming result, live `AssetKind::mavg_cluster_graph` catalog row, caller-owned graph document, and matching runtime mesh payload without executing package streaming or backend-specific LoD work.
- Docs, plans, registry, manifest fragments, composed manifest, and static checks describe exactly the implemented static conventional LOD scope.
- Full `tools/validate.ps1` passes or records a concrete host/tool blocker.

## Non-Claims

- No GPU culling.
- No D3D12 `ExecuteIndirect` or Vulkan indirect draw execution.
- No mesh/task/amplification shader readiness.
- No autonomous background package streaming worker, thread ownership, async overlap/performance claim, automatic eviction policy beyond caller-reviewed candidate order plus selected/fallback ancestor protection, partial `.mavgpayload` byte-range page loader/schema, or GPU memory pressure integration.
- No GPU memory residency enforcement beyond existing value/evidence rows.
- No skinned, morph, displacement, destruction, or dynamic cluster support.
- No ray tracing payload or BLAS/TLAS integration.
- No Metal readiness from Windows validation.
- No Nanite compatibility, Nanite-equivalence, Nanite-superiority, or benchmark-exceeds claim.

## Follow-Up Child Plans

After this milestone lands:

- `mavg-gpu-culling-indirect-v1`: completed stacked child for value-only packed indexed indirect command planning, reviewed culling bounds, and D3D12/Vulkan synchronization requirement rows; later stacked children own selected D3D12 backend indirect execution and selected D3D12 MAVG visibility compaction proof, while broad GPU culling frameworks plus Vulkan/Metal backend execution remain follow-up.
- `mavg-rhi-indirect-draw-v1`: active child for `indirect_draw.hpp`, `IndexedIndirectDrawCommand`, `IndexedIndirectDrawDesc`, `BufferUsage::indirect`, `IRhiCommandList::draw_indexed_indirect`, Null RHI argument/count-buffer execution counters, and Vulkan indirect-buffer usage mapping; D3D12 `ExecuteIndirect` and Vulkan indirect draw execution remain future backend work.
- `mavg-package-streaming-residency-v1`: now starts with caller-reviewed page request planning, selected/fallback ancestor eviction protection, and one-row safe-point drain; remaining follow-up is autonomous/background dispatch policy, partial payload page schema, automatic eviction policy, and GPU memory pressure integration.
- `mavg-mesh-shader-backends-v1`: D3D12 mesh/amplification shader path, Vulkan `VK_EXT_mesh_shader` path, strict feature gates, fallback preservation.
- `mavg-deformable-clusters-v1`: rigid, skinned, morph, and dynamic update tiers.
- `mavg-ray-tracing-consistency-v1`: raster/RT payload consistency and backend-local acceleration structure evidence.
- `mavg-benchmark-claim-ladder-v1`: first-party scenes, conventional path comparison, host matrix, trace evidence, and claim wording review.
