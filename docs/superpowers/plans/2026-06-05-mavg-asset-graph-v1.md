# 2026-06-05 MAVG Asset Graph v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` or focused inline TDD execution to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `mavg-asset-graph-v1`

**Status:** Active.

**Goal:** Implement the first MAVG code slice: deterministic clustered asset graph validation and deterministic first-party cook/package planning, without renderer, streaming, deformation, ray tracing, or benchmark superiority claims.

**Architecture:** Add a value-only `MK_assets` graph document for static MAVG clusters and pages, then add a dependency-free `MK_tools` cook planner that emits stable first-party package metadata for that graph. The slice validates hierarchy, material partitions, bounds, error rows, page rows, and resident fallback ancestors so later selection/streaming work cannot create visible holes from invalid source data.

**Tech Stack:** C++23, `MK_assets`, `MK_tools`, repository CMake targets, PowerShell validation scripts, composed `engine/agent/manifest.json`, and the 2026-06-05 MAVG architecture/benchmark specs.

---

## Context

Phase 0 (`mavg-research-legal-benchmark-baseline-v1`) completed the clean-room/legal, official-source, benchmark-methodology, stale-doc, first-party UI/editor, Win32/WASAPI, and performance-foundation audit. The benchmark claim ladder starts with static clustered asset graph validation, so this plan must not skip into runtime selection, renderer execution, streaming, GPU culling, mesh shaders, ray tracing, deformation, or benchmark-exceeds work.

## Constraints

- Keep `unsupportedProductionGaps = []`; this is post-1.0 feature work, not an Engine 1.0 blocker.
- Do not add third-party dependencies, datasets, simplifiers, codecs, or mesh optimizers.
- Do not copy or pattern-match Unreal Engine, Nanite, UE shaders, UE cooked data, private presentations, or reverse-engineered implementation details.
- Do not add SDL3, Dear ImGui, Qt, Slint, RmlUi, UI middleware, public native handles, or public RHI/backend handles.
- Do not claim MAVG is Nanite-compatible, Nanite-equivalent, Nanite-exceeding, renderer-ready, package-streaming-ready, mesh-shader-ready, Vulkan/Metal-ready, or broadly optimized.
- Use deterministic ordering for graph validation, serialization, cook outputs, diagnostics, changed files, and package rows.

## Files

- Create: `engine/assets/include/mirakana/assets/mavg_cluster_graph.hpp`
- Create: `engine/assets/src/mavg_cluster_graph.cpp`
- Create: `engine/tools/include/mirakana/tools/mavg_cluster_cook.hpp`
- Create: `engine/tools/asset/mavg_cluster_cook.cpp`
- Create: `tests/unit/mavg_cluster_graph_tests.cpp`
- Create: `tests/unit/tools_mavg_cluster_cook_tests.cpp`
- Modify: `engine/assets/CMakeLists.txt`
- Modify: `engine/tools/asset/CMakeLists.txt`
- Modify: `CMakeLists.txt`
- Modify: `engine/assets/include/mirakana/assets/asset_registry.hpp`
- Modify: `engine/assets/include/mirakana/assets/asset_dependency_graph.hpp`
- Modify: `engine/assets/src/asset_dependency_graph.cpp`
- Modify: `engine/assets/src/asset_package.cpp`
- Modify: `engine/assets/src/asset_identity.cpp`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Compose: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration-010-agent-baseline.ps1`
- Modify: `tools/check-ai-integration-020-engine-manifest.ps1`
- Modify: `tools/check-json-contracts-030-tooling-contracts.ps1`
- Modify: `tools/check-json-contracts-040-agent-surfaces.ps1`

## Public API Contract

`mirakana::AssetKind` gains `mavg_cluster_graph`.

`mirakana::AssetDependencyKind` gains graph-local dependency edge kinds for source mesh and material references:

- `mavg_source_mesh`
- `mavg_material`

`engine/assets/include/mirakana/assets/mavg_cluster_graph.hpp` owns these first-party value types and functions:

- `std::string_view mavg_cluster_graph_format_v1()`
- `MavgBounds3f`
- `MavgClusterGraphPage`
- `MavgClusterGraphMaterialPartition`
- `MavgClusterGraphCluster`
- `MavgClusterGraphDocument`
- `MavgClusterGraphDiagnosticCode`
- `MavgClusterGraphDiagnostic`
- `MavgClusterGraphValidationResult`
- `validate_mavg_cluster_graph(const MavgClusterGraphDocument&)`
- `canonicalize_mavg_cluster_graph(MavgClusterGraphDocument)`
- `serialize_mavg_cluster_graph_document(const MavgClusterGraphDocument&)`
- `deserialize_mavg_cluster_graph_document(std::string_view)`

`engine/tools/include/mirakana/tools/mavg_cluster_cook.hpp` owns these first-party tool rows and functions:

- `MavgClusterCookTriangle`
- `MavgClusterCookMaterialPartition`
- `MavgClusterCookRequest`
- `MavgClusterCookChangedFile`
- `MavgClusterCookPackageRow`
- `MavgClusterCookResult`
- `plan_mavg_cluster_graph_cook_package(const MavgClusterCookRequest&)`
- `apply_mavg_cluster_graph_cook_package(IFileSystem&, const MavgClusterCookRequest&)`

## Tasks

### Task 1: Add Failing Graph Contract Tests

**Files:**

- Create: `tests/unit/mavg_cluster_graph_tests.cpp`
- Modify: `CMakeLists.txt`

- [ ] Add a dedicated `MK_mavg_cluster_graph_tests` executable linked to `MK_assets` and including `tests`.
- [ ] Add tests for:
  - valid graph canonicalization and deterministic serialization
  - duplicate cluster ids
  - duplicate page ids
  - missing parent
  - parent cycle
  - child backlink mismatch
  - invalid bounds
  - invalid geometric error
  - invalid material partition reference
  - missing page reference
  - missing resident fallback ancestor
- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_cluster_graph_tests
```

Expected: fail before production files exist, with missing `mirakana/assets/mavg_cluster_graph.hpp` or missing symbol diagnostics.

### Task 2: Implement MAVG Cluster Graph v1

**Files:**

- Create: `engine/assets/include/mirakana/assets/mavg_cluster_graph.hpp`
- Create: `engine/assets/src/mavg_cluster_graph.cpp`
- Modify: `engine/assets/CMakeLists.txt`
- Modify: `engine/assets/include/mirakana/assets/asset_registry.hpp`
- Modify: `engine/assets/include/mirakana/assets/asset_dependency_graph.hpp`
- Modify: `engine/assets/src/asset_dependency_graph.cpp`
- Modify: `engine/assets/src/asset_package.cpp`
- Modify: `engine/assets/src/asset_identity.cpp`

- [ ] Define the graph document and diagnostic API listed in **Public API Contract**.
- [ ] Canonicalize pages, material partitions, and clusters by stable ids before validation output and serialization.
- [ ] Reject invalid rows with deterministic diagnostics:
  - empty document
  - duplicate page id
  - duplicate material partition id
  - duplicate cluster id
  - invalid bounds
  - invalid non-finite or negative geometric error
  - invalid vertex or triangle counts
  - missing material partition
  - missing page
  - missing parent
  - self parent
  - cycle
  - child backlink mismatch
  - parent error smaller than child error
  - missing resident fallback ancestor
- [ ] Serialize as `GameEngine.MavgClusterGraph.v1` text with sorted rows and no locale-dependent formatting.
- [ ] Deserialize only that exact format id and fail closed on malformed rows.
- [ ] Add `AssetKind::mavg_cluster_graph` and dependency kind text for `mavg_source_mesh` / `mavg_material`.
- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_cluster_graph_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_mavg_cluster_graph_tests
```

Expected: `MK_mavg_cluster_graph_tests` passes.

### Task 3: Add Failing Cook/Package Planning Tests

**Files:**

- Create: `tests/unit/tools_mavg_cluster_cook_tests.cpp`
- Modify: `CMakeLists.txt`

- [ ] Add a dedicated `MK_tools_mavg_cluster_cook_tests` executable linked to `MK_tools`.
- [ ] Add tests for:
  - shuffled but equivalent triangle input produces identical graph bytes and changed-file hashes
  - package output uses `AssetKind::mavg_cluster_graph`
  - unsafe output paths are rejected
  - zero source revision is rejected
  - empty mesh is rejected
  - duplicate material partitions are rejected
  - unsupported renderer/RHI/streaming/deformation/ray-tracing/mesh-shader/benchmark/native-handle/free-form claim rows are rejected
  - produced graph passes `validate_mavg_cluster_graph`
- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_tools_mavg_cluster_cook_tests
```

Expected: fail before cook header/source files exist, with missing `mirakana/tools/mavg_cluster_cook.hpp` or missing symbol diagnostics.

### Task 4: Implement Deterministic Cook/Package Planner

**Files:**

- Create: `engine/tools/include/mirakana/tools/mavg_cluster_cook.hpp`
- Create: `engine/tools/asset/mavg_cluster_cook.cpp`
- Modify: `engine/tools/asset/CMakeLists.txt`

- [ ] Define `MavgClusterCookRequest` with source mesh asset id, output graph asset id, source revision, output graph/package paths, material partitions, triangle rows, max triangles per leaf, and explicit unsupported-claim rows.
- [ ] Define `MavgClusterCookResult` with graph document, changed files, package rows, diagnostics, validation recipe ids, unsupported gap ids, replay hash, and `ready` flag.
- [ ] Sort triangles by material partition and stable triangle id before grouping.
- [ ] Generate deterministic cluster ids, page ids, parent rows, child backlink rows, material partition rows, bounds, error values, resident fallback rows, and package index metadata.
- [ ] Reject unsafe paths and unsupported claim rows before emitting changed files.
- [ ] Keep the planner offline and value-only; `apply_mavg_cluster_graph_cook_package` may write only the reviewed changed files through `IFileSystem`.
- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_tools_mavg_cluster_cook_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_mavg_cluster_graph_tests|MK_tools_mavg_cluster_cook_tests"
```

Expected: both focused MAVG CTest targets pass.

### Task 5: Sync Docs, Manifest, And Static Contracts

**Files:**

- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Compose: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration-010-agent-baseline.ps1`
- Modify: `tools/check-ai-integration-020-engine-manifest.ps1`
- Modify: `tools/check-json-contracts-030-tooling-contracts.ps1`
- Modify: `tools/check-json-contracts-040-agent-surfaces.ps1`

- [ ] Record `mavg-asset-graph-v1` as the active plan while implementation is in progress.
- [ ] Record Phase 0 as completed evidence, not an active slice.
- [ ] Add `mavg_cluster_graph.hpp`, `mavg_cluster_cook.hpp`, `GameEngine.MavgClusterGraph.v1`, and `AssetKind::mavg_cluster_graph` to current-truth surfaces only after code/tests are green.
- [ ] Keep non-claims for CPU selector, renderer execution, GPU culling, mesh shaders, streaming/residency, deformation, ray tracing, benchmark runner/results, Nanite compatibility/equivalence/superiority, public native handles, SDL3, Dear ImGui, and broad optimization.
- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
```

Expected: all checks pass.

### Task 6: Slice Validation And Publication

**Files:**

- Validate all touched code, docs, manifest, static checks, and generated manifest output.

- [ ] Run focused C++ validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_cluster_graph_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_tools_mavg_cluster_cook_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_mavg_cluster_graph_tests|MK_tools_mavg_cluster_cook_tests"
```

- [ ] Run full slice validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
git diff --check
```

- [ ] Run publication preflight before staging, push, or PR:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1
```

Expected: focused tests, full validation, whitespace check, and publication preflight pass or record a concrete blocker.

## Done When

- `MK_assets` exposes deterministic `MavgClusterGraphDocument` validation, canonicalization, serialization, and deserialization.
- `MK_tools` exposes deterministic MAVG cluster cook/package planning and reviewed file application through `IFileSystem`.
- Focused graph and cook CTest targets pass.
- Docs, registry, manifest fragments, composed manifest, and static contracts reflect the new implemented foundation without broad claims.
- `unsupportedProductionGaps = []` remains unchanged.
- No third-party dependency/legal records change unless a future implementation deliberately adds an audited dependency.
- Full `tools/validate.ps1` passes or records a concrete host/tool blocker.

## Non-Claims

This plan does not implement or claim:

- CPU selector
- renderer adoption
- GPU culling
- indirect draws
- mesh shaders
- streaming/residency execution
- deformation
- ray tracing
- benchmark runner or benchmark results
- Nanite compatibility, equivalence, or superiority
- broad renderer quality
- Vulkan or Metal readiness by inference
- public native/RHI/backend handles
- SDL3, Dear ImGui, or UI middleware
- broad CPU/GPU/memory optimization
