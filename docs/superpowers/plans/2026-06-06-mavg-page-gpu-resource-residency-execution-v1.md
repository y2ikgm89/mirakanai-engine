# MAVG Page GPU Resource Residency Execution v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a narrow `MK_runtime_rhi` handoff helper that executes existing MAVG page residency actions from a completed `RuntimeMavgPageGpuResourceUpdateReadinessResult`.

**Architecture:** The helper validates #505 readiness evidence, then delegates to `execute_runtime_mavg_page_residency_actions` without duplicating residency policy or RHI backend logic. It does not submit DirectStorage work, allocate RHI resources, mutate mounts, enforce allocator budgets, schedule command-list residency sets, or claim a full page-to-resource residency service.

**Tech Stack:** C++23, `MK_runtime_rhi`, `MK_runtime`, `MK_rhi`, existing Null RHI residency tests, Microsoft Learn D3D12 `ID3D12Device::MakeResident` / `Evict` API reference, CMake/CTest, PowerShell validation wrappers.

---

## Context

- PR #505 adds `make_runtime_mavg_page_gpu_resource_update_readiness`, which emits `RuntimeMavgResidentPageResourceRow` values after completed caller-owned RHI DirectStorage resource-destination evidence.
- `execute_runtime_mavg_page_residency_actions` already validates resident page resources, selected clusters, protected mounts, and reviewed eviction candidates, then calls `IRhiDevice::execute_residency_action`.
- Microsoft Learn documents D3D12 `MakeResident` / `Evict` as residency APIs over `ID3D12Pageable*` arrays and restricts residency changes to supported pageable objects such as committed resources and heaps.
- This candidate only bridges #505 output into the existing residency action adapter. It is not GPU decompression, allocator budget enforcement, or a full lifecycle service.

## Files

- Create: `engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_page_gpu_resource_residency_execution.hpp`
- Create: `engine/runtime_rhi/src/mavg_page_gpu_resource_residency_execution.cpp`
- Create: `tests/unit/runtime_rhi_mavg_page_gpu_resource_residency_execution_tests.cpp`
- Modify: `engine/runtime_rhi/CMakeLists.txt`
- Modify: `CMakeLists.txt`
- Update: `docs/current-capabilities.md`
- Update: `docs/roadmap.md`
- Update: `docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md`
- Update: `docs/superpowers/plans/README.md`
- Update: `engine/agent/manifest.fragments/004-modules.json`
- Update: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Compose: `engine/agent/manifest.json`
- Create: `tools/check-ai-integration-133-mavg-page-gpu-resource-residency-execution.ps1`

## Public API Shape

Add this header contract:

```cpp
namespace mirakana::runtime_rhi {

enum class RuntimeMavgPageGpuResourceResidencyExecutionDiagnosticCode : std::uint8_t {
    invalid_graph_asset = 0,
    missing_graph,
    invalid_graph,
    missing_readiness_result,
    invalid_readiness_result,
    readiness_not_ready,
    residency_action_failed,
};

struct RuntimeMavgPageGpuResourceResidencyExecutionDiagnostic {
    RuntimeMavgPageGpuResourceResidencyExecutionDiagnosticCode code{
        RuntimeMavgPageGpuResourceResidencyExecutionDiagnosticCode::invalid_graph_asset};
    AssetId graph_asset;
    std::uint32_t page_index{0};
    runtime::RuntimeResidentPackageMountIdV2 mount_id;
    std::string message;
};

struct RuntimeMavgPageGpuResourceResidencyExecutionDesc {
    AssetId graph_asset;
    const MavgClusterGraphDocument* graph{nullptr};
    const RuntimeMavgPageGpuResourceUpdateReadinessResult* readiness_result{nullptr};
    std::span<const runtime::RuntimeMavgPageStreamingSelectedClusterRow> selected_clusters;
    std::span<const runtime::RuntimeResidentPackageMountIdV2> protected_mount_ids;
    std::span<const runtime::RuntimeResidentPackageMountIdV2> eviction_candidate_unmount_order;
    bool make_selected_pages_resident{true};
    bool evict_reviewed_candidates{true};
};

struct RuntimeMavgPageGpuResourceResidencyExecutionResult {
    std::vector<RuntimeMavgPageGpuResourceResidencyExecutionDiagnostic> diagnostics;
    RuntimeMavgPageResidencyActionResult residency_action_result;
    std::size_t input_ready_resource_count{0};
    std::size_t selected_page_resource_count{0};
    std::size_t protected_page_resource_count{0};
    std::size_t eviction_candidate_resource_count{0};
    std::size_t made_resident_count{0};
    std::size_t evicted_count{0};
    std::size_t protected_skip_count{0};
    bool consumed_gpu_resource_update_readiness{false};
    bool used_directstorage_resource_destination{false};
    bool used_directstorage_caller_owned_rhi_resource_destination{false};
    bool directstorage_status_complete{false};
    bool observed_native_queue_submission{false};
    bool invoked_rhi_residency_action{false};
    bool invoked_make_resident_action{false};
    bool invoked_evict_action{false};
    bool invoked_native_make_resident{false};
    bool invoked_native_evict{false};
    bool invoked_file_io{false};
    bool submitted_native_queue{false};
    bool allocated_rhi_resources{false};
    bool enforced_allocator_budget{false};
    bool mutated_mount_set{false};
    bool used_gpu_decompression{false};
    bool exposed_native_handles{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty() && residency_action_result.succeeded();
    }
};

[[nodiscard]] RuntimeMavgPageGpuResourceResidencyExecutionResult
execute_runtime_mavg_page_gpu_resource_residency_actions(
    rhi::IRhiDevice& device,
    const RuntimeMavgPageGpuResourceResidencyExecutionDesc& desc);

} // namespace mirakana::runtime_rhi
```

## Task 1: RED Handoff Tests

- [x] Create `tests/unit/runtime_rhi_mavg_page_gpu_resource_residency_execution_tests.cpp`.
- [x] Add a success test that creates:
  - a valid three-page graph
  - a `RuntimeMavgPageGpuResourceUpdateReadinessResult` with three `RuntimeMavgResidentPageResourceRow` buffer resources
  - selected cluster row for page 1
  - protected mount id for page 0
  - eviction candidates for page 0 and page 2
  - a `mirakana::rhi::NullRhiDevice`
- [x] Assert:

```cpp
MK_REQUIRE(result.succeeded());
MK_REQUIRE(result.consumed_gpu_resource_update_readiness);
MK_REQUIRE(result.input_ready_resource_count == 3U);
MK_REQUIRE(result.selected_page_resource_count == 1U);
MK_REQUIRE(result.protected_page_resource_count == 1U);
MK_REQUIRE(result.eviction_candidate_resource_count == 1U);
MK_REQUIRE(result.made_resident_count == 2U);
MK_REQUIRE(result.evicted_count == 1U);
MK_REQUIRE(result.protected_skip_count == 1U);
MK_REQUIRE(result.invoked_rhi_residency_action);
MK_REQUIRE(result.invoked_make_resident_action);
MK_REQUIRE(result.invoked_evict_action);
MK_REQUIRE(result.used_directstorage_resource_destination);
MK_REQUIRE(result.used_directstorage_caller_owned_rhi_resource_destination);
MK_REQUIRE(result.directstorage_status_complete);
MK_REQUIRE(result.observed_native_queue_submission);
```

- [x] Assert non-claims stay false:

```cpp
MK_REQUIRE(!result.invoked_file_io);
MK_REQUIRE(!result.submitted_native_queue);
MK_REQUIRE(!result.allocated_rhi_resources);
MK_REQUIRE(!result.invoked_native_make_resident);
MK_REQUIRE(!result.invoked_native_evict);
MK_REQUIRE(!result.enforced_allocator_budget);
MK_REQUIRE(!result.mutated_mount_set);
MK_REQUIRE(!result.used_gpu_decompression);
MK_REQUIRE(!result.exposed_native_handles);
```

- [x] Add fail-closed tests for missing readiness result, readiness result with diagnostics, readiness result not ready, and selected pages without ready resource rows.
- [x] Add the test target to root `CMakeLists.txt`.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_page_gpu_resource_residency_execution_tests
```

Expected: compile failure because `mirakana/runtime_rhi/mavg_page_gpu_resource_residency_execution.hpp` and the helper symbols do not exist.

## Task 2: GREEN Handoff API And Implementation

- [x] Add `mavg_page_gpu_resource_residency_execution.hpp` with the public API shape above.
- [x] Implement `mavg_page_gpu_resource_residency_execution.cpp` to validate:
  - non-zero `graph_asset`
  - present graph
  - graph asset matches and validates
  - present readiness result
  - readiness result has no diagnostics
  - readiness result has `ready_for_residency_actions=true`
  - readiness result has `resident_page_resources` rows
- [x] Delegate to `execute_runtime_mavg_page_residency_actions` with:
  - `desc.graph_asset`
  - `desc.graph`
  - `desc.selected_clusters`
  - `readiness_result->resident_page_resources`
  - `desc.protected_mount_ids`
  - `desc.eviction_candidate_unmount_order`
  - `desc.make_selected_pages_resident`
  - `desc.evict_reviewed_candidates`
- [x] Copy counts and evidence flags from readiness and residency action result into the new result.
- [x] If the delegated residency action result fails, add `residency_action_failed` and do not hide the delegated diagnostics.
- [x] Keep helper-owned non-claims explicit:
  - `invoked_file_io=false`
  - `submitted_native_queue=false`
  - `allocated_rhi_resources=false`
  - `enforced_allocator_budget=false`
  - `mutated_mount_set=false`
  - `exposed_native_handles=false` unless delegated RHI evidence sets it
- [x] Add the source to `engine/runtime_rhi/CMakeLists.txt`.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_page_gpu_resource_residency_execution_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_rhi_mavg_page_gpu_resource_residency_execution_tests
```

Expected: PASS.

## Task 3: Docs, Manifest, And Static Guard

- [x] Update current capabilities, roadmap, MAVG master plan, and plan registry with the new handoff evidence and non-claims.
- [x] Update `engine/agent/manifest.fragments/004-modules.json` and `010-aiOperableProductionLoop.json`.
- [x] Compose the manifest:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
```

- [x] Add `tools/check-ai-integration-133-mavg-page-gpu-resource-residency-execution.ps1` to assert the new API, test target, docs, manifest evidence, and non-claims.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
```

## Task 4: Validation And Publication

- [x] Run focused related tests:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_page_gpu_resource_residency_execution_tests MK_runtime_rhi_mavg_page_gpu_resource_update_tests MK_runtime_rhi_mavg_residency_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_rhi_mavg_page_gpu_resource_residency_execution_tests|MK_runtime_rhi_mavg_page_gpu_resource_update_tests|MK_runtime_rhi_mavg_residency_tests"
```

- [x] Run slice checks:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1
```

- [ ] Commit, push, and open a stacked draft PR against `codex/mavg-page-gpu-resource-update-readiness-v1`.

## Done When

- `MK_runtime_rhi` can consume completed page GPU resource update readiness rows and execute existing MAVG page residency actions.
- Selected/protected ready pages are made resident, reviewed unprotected eviction candidates are evicted, and protected eviction candidates are skipped.
- Missing/invalid/not-ready readiness evidence and delegated residency validation failures fail before unsupported side effects.
- Docs, manifest fragments, composed manifest, static guards, focused tests, and full validation match the exact narrow evidence.
- DirectStorage queue submission by this helper, file IO, RHI allocation, mount mutation, allocator/GPU budget enforcement, full page-to-resource lifecycle ownership, command-list residency-set scheduling, GPU decompression, Vulkan/Metal native IO parity, mesh shaders, deformation, ray tracing, async-overlap/performance, benchmark superiority, and Nanite compatibility/equivalence/superiority remain unclaimed.

## Validation Evidence

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_page_gpu_resource_residency_execution_tests` failed on missing `mirakana/runtime_rhi/mavg_page_gpu_resource_residency_execution.hpp`.
- GREEN: target build and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_rhi_mavg_page_gpu_resource_residency_execution_tests` passed.
- Docs/manifest/static: `tools/compose-agent-manifest.ps1 -Write`, `tools/check-ai-integration.ps1`, `tools/check-json-contracts.ps1`, and `tools/check-public-api-boundaries.ps1` passed.
- Focused regression: related `MK_runtime_rhi_mavg_page_gpu_resource_residency_execution_tests`, `MK_runtime_rhi_mavg_page_gpu_resource_update_tests`, and `MK_runtime_rhi_mavg_residency_tests` build and CTest passed.
- Slice gate: `tools/check-format.ps1` and full `tools/validate.ps1` passed; validation ran 113/113 tests successfully.
