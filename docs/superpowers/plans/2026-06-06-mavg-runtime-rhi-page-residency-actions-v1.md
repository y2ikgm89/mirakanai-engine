# MAVG Runtime RHI Page Residency Actions v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a narrow MAVG page-to-RHI residency action adapter that turns reviewed resident page/resource rows and protected eviction evidence into `IRhiDevice::execute_residency_action` calls.

**Architecture:** Keep MAVG page ownership in `MK_runtime` values and GPU residency mutation behind `MK_runtime_rhi` plus the existing backend-neutral `IRhiDevice` residency action boundary. The adapter validates graph/page/mount/resource rows, makes selected/protected pages resident, evicts only reviewed unprotected page resources, and records evidence without exposing native handles, enforcing allocator budgets, or claiming async overlap.

**Tech Stack:** C++23, `MK_runtime`, `MK_runtime_rhi`, `MK_rhi`, CMake target-based source/test wiring, Null RHI deterministic tests, D3D12 WARP residency action evidence, Microsoft Learn D3D12 `ID3D12Device::MakeResident` / `ID3D12Device::Evict` constraints, and Context7 `/kitware/cmake` target-based build guidance.

---

## Context

- Base branch: `codex/mavg-directstorage-native-execution-v1`.
- Candidate branch: `codex/mavg-runtime-rhi-page-residency-actions-v1`.
- Current MAVG support already includes resident page rows, automatic eviction review rows, DXGI pressure rows, and `IRhiDevice::execute_residency_action`.
- Microsoft Learn documents that D3D12 residency changes are supported only for objects such as descriptor heaps, heaps, committed resources, and query heaps; `ID3D12Device::MakeResident` can fail, and applications must use fences so non-resident resources are not used by the GPU. The inverse native action is `ID3D12Device::Evict`.
- The implementation must call the existing RHI abstraction, not D3D12 APIs directly.

## Non-Goals

- No DirectStorage native fence signaling.
- No D3D12 resource-destination DirectStorage IO.
- No GPU decompression.
- No allocator/GPU budget enforcement or video-memory reservation.
- No command-list residency-set scheduling.
- No async-overlap or performance claim.
- No Vulkan/Metal parity claim.
- No mesh shader, deformation, ray tracing, or Nanite equivalence/superiority claim.

## Files

- Create: `engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_residency.hpp`
- Create: `engine/runtime_rhi/src/mavg_residency.cpp`
- Modify: `engine/runtime_rhi/CMakeLists.txt`
- Create: `tests/unit/runtime_rhi_mavg_residency_tests.cpp`
- Modify: `tests/unit/d3d12_rhi_tests.cpp`
- Modify: `CMakeLists.txt`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Generate: `engine/agent/manifest.json`
- Add: `tools/check-ai-integration-128-mavg-runtime-rhi-page-residency-actions.ps1`

## Public API Draft

```cpp
namespace mirakana::runtime_rhi {

enum class RuntimeMavgPageResidencyActionDiagnosticCode : std::uint8_t {
    invalid_graph_asset = 0,
    missing_graph,
    invalid_graph,
    invalid_resident_page_resource,
    duplicate_resident_page_resource,
    missing_resident_page_resource,
    invalid_selected_cluster,
    invalid_protected_mount,
    duplicate_protected_mount,
    invalid_eviction_candidate,
    duplicate_eviction_candidate,
    rhi_make_resident_failed,
    rhi_evict_failed,
};

struct RuntimeMavgResidentPageResourceRow {
    AssetId graph_asset;
    std::uint32_t page_index{0};
    runtime::RuntimeResidentPackageMountIdV2 mount_id;
    rhi::RhiResidencyResourceRef resource;
    std::uint64_t estimated_gpu_resident_bytes{0};
};

struct RuntimeMavgPageResidencyActionDesc {
    AssetId graph_asset;
    const MavgClusterGraphDocument* graph{nullptr};
    std::span<const runtime::RuntimeMavgPageStreamingSelectedClusterRow> selected_clusters;
    std::span<const RuntimeMavgResidentPageResourceRow> resident_page_resources;
    std::span<const runtime::RuntimeResidentPackageMountIdV2> protected_mount_ids;
    std::span<const runtime::RuntimeResidentPackageMountIdV2> eviction_candidate_unmount_order;
    bool make_selected_pages_resident{true};
    bool evict_reviewed_candidates{true};
};

struct RuntimeMavgPageResidencyActionResult {
    std::vector<RuntimeMavgPageResidencyActionDiagnostic> diagnostics;
    std::vector<RuntimeMavgPageResidencyActionRow> action_rows;
    rhi::RhiResidencyActionResult make_resident_result;
    rhi::RhiResidencyActionResult evict_result;
    std::size_t input_resident_page_resource_count{0};
    std::size_t selected_page_resource_count{0};
    std::size_t protected_page_resource_count{0};
    std::size_t eviction_candidate_resource_count{0};
    std::size_t made_resident_count{0};
    std::size_t evicted_count{0};
    std::size_t protected_skip_count{0};
    bool invoked_rhi_residency_action{false};
    bool invoked_make_resident_action{false};
    bool invoked_evict_action{false};
    bool invoked_native_make_resident{false};
    bool invoked_native_evict{false};
    bool exposed_native_handles{false};
    bool enforced_allocator_budget{false};
    bool invoked_file_io{false};
    bool mutated_mount_set{false};
    bool used_directstorage_resource_destination{false};
    bool used_gpu_decompression{false};
    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] RuntimeMavgPageResidencyActionResult
execute_runtime_mavg_page_residency_actions(rhi::IRhiDevice& device, const RuntimeMavgPageResidencyActionDesc& desc);

} // namespace mirakana::runtime_rhi
```

## Tasks

### Task 1: Add Failing Null RHI Tests

**Files:**
- Create: `tests/unit/runtime_rhi_mavg_residency_tests.cpp`
- Modify: `CMakeLists.txt`

- [x] Add tests for successful selected/protected make-resident plus reviewed unprotected eviction.
- [x] Add tests for missing resource rows and duplicate page/mount rows failing before RHI calls.
- [x] Add tests proving protected eviction candidates are skipped.
- [x] Register `MK_runtime_rhi_mavg_residency_tests` linking `MK_runtime_rhi`.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_residency_tests
```

Expected: configure succeeds; build fails because `mirakana/runtime_rhi/mavg_residency.hpp` or `execute_runtime_mavg_page_residency_actions` is missing.

Evidence: RED passed as expected: configure succeeded and `MK_runtime_rhi_mavg_residency_tests` initially failed to build because `mirakana/runtime_rhi/mavg_residency.hpp` was missing.

### Task 2: Implement Runtime RHI Adapter

**Files:**
- Create: `engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_residency.hpp`
- Create: `engine/runtime_rhi/src/mavg_residency.cpp`
- Modify: `engine/runtime_rhi/CMakeLists.txt`

- [x] Add value-only public types and `execute_runtime_mavg_page_residency_actions`.
- [x] Validate non-zero graph asset, non-null valid graph, graph asset match, known graph pages, non-zero mount ids, positive estimated bytes, and valid RHI resource rows.
- [x] Reject duplicate resident resource rows by page index or mount id.
- [x] Build selected/protected make-resident resources from selected clusters and protected mount ids.
- [x] Build evict resources from reviewed candidate mount ids after removing protected mount ids.
- [x] Call `device.execute_residency_action` once for make-resident rows and once for evict rows only when those row sets are non-empty.
- [x] Propagate `RhiResidencyActionResult` status and native evidence fields into the result.
- [x] Keep `exposed_native_handles=false`, `enforced_allocator_budget=false`, `invoked_file_io=false`, `mutated_mount_set=false`, `used_directstorage_resource_destination=false`, and `used_gpu_decompression=false`.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_residency_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_rhi_mavg_residency_tests
```

Expected: target builds and the new test passes.

Evidence: GREEN passed with `tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_residency_tests` and `tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_rhi_mavg_residency_tests`.

### Task 3: Add D3D12 WARP Evidence

**Files:**
- Modify: `tests/unit/d3d12_rhi_tests.cpp`

- [x] Add a D3D12 test that creates committed buffer/texture resources, maps them to MAVG resident page resources, and executes the new adapter.
- [x] Assert `invoked_native_make_resident=true`, `invoked_native_evict=true`, `made_resident_count`, `evicted_count`, `protected_skip_count`, and no native handle exposure or allocator enforcement.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_d3d12_rhi_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_d3d12_rhi_tests
```

Expected: D3D12 WARP tests pass.

Evidence: GREEN passed with `tools/cmake.ps1 --build --preset dev --target MK_d3d12_rhi_tests` and `tools/ctest.ps1 --preset dev --output-on-failure -R MK_d3d12_rhi_tests`.

### Task 4: Update Docs, Manifest, Static Guard

**Files:**
- Modify docs and manifest fragments listed above.
- Add: `tools/check-ai-integration-128-mavg-runtime-rhi-page-residency-actions.ps1`

- [x] Update current capability wording to include the narrow page-to-RHI residency adapter and keep allocator/performance/Nanite non-claims.
- [x] Update the MAVG spec/master plan and plan registry with this child as active during implementation.
- [x] Add manifest fragment evidence and retained plan paths.
- [x] Compose the manifest:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
```

- [x] Add a static guard that verifies the new API names, non-claim literals, plan path, and manifest evidence.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
```

Expected: all checks pass.

Evidence: Passed `tools/check-ai-integration.ps1`, `tools/check-json-contracts.ps1`, and `tools/check-agents.ps1` after composing `engine/agent/manifest.json`.

### Task 5: Slice Validation And Publication

**Files:**
- All task-owned changes.

- [x] Run focused public/API and formatting checks:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
git diff --check
```

Evidence: Passed `tools/check-public-api-boundaries.ps1`, `tools/check-format.ps1`, `tools/check-text-format.ps1`, and `git diff --check` after running `tools/format.ps1` for C++ formatting.

- [x] Run full validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Evidence: Passed full `tools/validate.ps1`. Static checks, build, and all 110 CTest tests passed; Apple/Metal checks remained host-gated/diagnostic-only on Windows as expected.

- [x] Commit only task-owned files after validation passes.
- [x] Run publication preflight, push, and open a stacked draft PR against `codex/mavg-directstorage-native-execution-v1`.

Evidence: Created commit `e8e3deb1` (`Add MAVG runtime RHI page residency actions`), pushed `codex/mavg-runtime-rhi-page-residency-actions-v1`, and opened stacked draft PR `https://github.com/y2ikgm89/mirakanai-engine/pull/498` against `codex/mavg-directstorage-native-execution-v1`.

## Done When

- `MK_runtime_rhi_mavg_residency_tests` proves deterministic adapter behavior on Null RHI.
- `MK_d3d12_rhi_tests` proves the adapter reaches native D3D12 `MakeResident` and `Evict` through `IRhiDevice`.
- Docs, plans, manifest fragments, composed manifest, and static checks describe the new support and remaining non-claims.
- Full `tools/validate.ps1` passes or a concrete host/tool blocker is recorded.
- A validated commit is pushed to a stacked draft PR.
