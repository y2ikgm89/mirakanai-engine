# MAVG Page GPU Resource Update Readiness v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a value-only `MK_runtime_rhi` readiness planner that turns completed caller-owned DirectStorage/RHI buffer destination evidence into `RuntimeMavgResidentPageResourceRow` rows for the existing MAVG residency action adapter.

**Architecture:** Keep DirectStorage submission, status polling, native handles, RHI allocation, mount mutation, and `MakeResident` / `Evict` execution outside this slice. The new planner consumes a successful `RuntimeMavgPageBufferDestinationPlanResult`, a successful native IO dispatch result, and a complete native IO status result, then validates byte/count/evidence consistency before emitting deterministic resident page resource/update readiness rows. The rows can feed `execute_runtime_mavg_page_residency_actions`, but this function only reports readiness.

**Tech Stack:** C++23, `MK_runtime_rhi`, `MK_runtime`, `MK_rhi`, CMake/CTest, PowerShell validation wrappers, Context7 Microsoft Learn lookup, and official Microsoft Learn DirectStorage Win32 API reference for status completion and buffer destination constraints.

---

## Context

- Official Microsoft Learn documents `DSTORAGE_DESTINATION_BUFFER` as a D3D12 resource destination with `Resource`, byte `Offset`, and byte `Size`.
- Official Microsoft Learn documents `IDStorageStatusArray` as receiving completion results for queued read requests, with `GetHResult` returning `E_PENDING` while requests are incomplete and `S_OK` after successful completion.
- PR #504 adds `RuntimeMavgPageBufferDestinationPlanResult` rows that map page requests to caller-owned first-party `BufferHandle` destination ranges, but it does not produce `RuntimeMavgResidentPageResourceRow` rows or inspect dispatch/status evidence.
- This child bridges that value-only gap without requiring `dstorageConfig.cmake`, the optional DirectStorage SDK host lane, or a real native submission on this host.

## Files

- Create: `engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_page_gpu_resource_update.hpp`
- Create: `engine/runtime_rhi/src/mavg_page_gpu_resource_update.cpp`
- Create: `tests/unit/runtime_rhi_mavg_page_gpu_resource_update_tests.cpp`
- Modify: `engine/runtime_rhi/CMakeLists.txt`
- Modify: `CMakeLists.txt`
- Update: `docs/current-capabilities.md`
- Update: `docs/roadmap.md`
- Update: `docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md`
- Update: `docs/superpowers/plans/README.md`
- Update: `engine/agent/manifest.fragments/004-modules.json`
- Update: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Compose: `engine/agent/manifest.json`
- Create: `tools/check-ai-integration-132-mavg-page-gpu-resource-update-readiness.ps1`

## Public API Shape

Add this header contract:

```cpp
namespace mirakana::runtime_rhi {

enum class RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode : std::uint8_t {
    invalid_graph_asset = 0,
    missing_buffer_destination_plan,
    invalid_buffer_destination_plan,
    missing_dispatch_result,
    invalid_dispatch_result,
    missing_status_result,
    incomplete_status_result,
    failed_status_result,
    resource_destination_not_used,
    submitted_request_count_mismatch,
    submitted_destination_bytes_mismatch,
    status_destination_bytes_mismatch,
    invalid_destination_row,
    duplicate_destination_row,
    ticket_mismatch,
};

struct RuntimeMavgPageGpuResourceUpdateReadinessDiagnostic {
    RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode code{
        RuntimeMavgPageGpuResourceUpdateReadinessDiagnosticCode::invalid_graph_asset};
    AssetId graph_asset;
    std::uint32_t page_index{0};
    runtime::RuntimeResidentPackageMountIdV2 mount_id;
    std::string message;
};

struct RuntimeMavgPageGpuResourceUpdateRow {
    AssetId graph_asset;
    std::uint32_t request_index{0};
    std::uint32_t page_index{0};
    runtime::RuntimeResidentPackageMountIdV2 mount_id;
    rhi::BufferHandle buffer;
    std::uint64_t destination_offset{0};
    std::uint32_t destination_size{0};
    std::uint64_t destination_range_offset{0};
    std::uint64_t destination_range_size{0};
    std::uint64_t estimated_gpu_resident_bytes{0};
    bool directstorage_resource_destination_complete{false};
    bool ready_for_residency_action{false};
};

struct RuntimeMavgPageGpuResourceUpdateReadinessDesc {
    AssetId graph_asset;
    const RuntimeMavgPageBufferDestinationPlanResult* buffer_destination_plan{nullptr};
    const runtime::RuntimeMavgPayloadNativeIoDispatchResult* dispatch_result{nullptr};
    const runtime::RuntimeMavgPayloadNativeIoStatusPollResult* status_result{nullptr};
};

struct RuntimeMavgPageGpuResourceUpdateReadinessResult {
    std::vector<RuntimeMavgResidentPageResourceRow> resident_page_resources;
    std::vector<RuntimeMavgPageGpuResourceUpdateRow> update_rows;
    std::vector<RuntimeMavgPageGpuResourceUpdateReadinessDiagnostic> diagnostics;
    std::size_t input_destination_row_count{0};
    std::size_t ready_resource_count{0};
    std::size_t duplicate_destination_row_count{0};
    std::uint64_t ready_destination_bytes{0};
    std::uint64_t ready_estimated_gpu_resident_bytes{0};
    bool used_directstorage_resource_destination{false};
    bool used_directstorage_caller_owned_rhi_resource_destination{false};
    bool directstorage_status_complete{false};
    bool observed_native_queue_submission{false};
    bool ready_for_residency_actions{false};
    bool invoked_file_io{false};
    bool submitted_native_queue{false};
    bool allocated_rhi_resources{false};
    bool invoked_rhi_residency_action{false};
    bool invoked_native_make_resident{false};
    bool invoked_native_evict{false};
    bool enforced_allocator_budget{false};
    bool mutated_mount_set{false};
    bool used_gpu_decompression{false};
    bool exposed_native_handles{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

[[nodiscard]] RuntimeMavgPageGpuResourceUpdateReadinessResult
make_runtime_mavg_page_gpu_resource_update_readiness(
    const RuntimeMavgPageGpuResourceUpdateReadinessDesc& desc);

} // namespace mirakana::runtime_rhi
```

## Task 1: RED Readiness Tests

- [x] Create `tests/unit/runtime_rhi_mavg_page_gpu_resource_update_tests.cpp`.
- [x] Add a success test with a synthetic two-page `RuntimeMavgPageBufferDestinationPlanResult`, a successful `RuntimeMavgPayloadNativeIoDispatchResult`, and a complete `RuntimeMavgPayloadNativeIoStatusPollResult`.
- [x] Assert that the result emits two `RuntimeMavgResidentPageResourceRow` rows in request order:

```cpp
MK_REQUIRE(result.succeeded());
MK_REQUIRE(result.ready_resource_count == 2U);
MK_REQUIRE(result.resident_page_resources[0].resource.kind == mirakana::rhi::RhiResidencyResourceKind::buffer);
MK_REQUIRE(result.resident_page_resources[0].resource.buffer.value == payload_buffer.value);
MK_REQUIRE(result.update_rows[0].directstorage_resource_destination_complete);
MK_REQUIRE(result.update_rows[0].ready_for_residency_action);
MK_REQUIRE(result.observed_native_queue_submission);
MK_REQUIRE(result.ready_for_residency_actions);
```

- [x] Assert non-claims stay false:

```cpp
MK_REQUIRE(!result.invoked_file_io);
MK_REQUIRE(!result.submitted_native_queue);
MK_REQUIRE(!result.allocated_rhi_resources);
MK_REQUIRE(!result.invoked_rhi_residency_action);
MK_REQUIRE(!result.invoked_native_make_resident);
MK_REQUIRE(!result.invoked_native_evict);
MK_REQUIRE(!result.enforced_allocator_budget);
MK_REQUIRE(!result.mutated_mount_set);
MK_REQUIRE(!result.used_gpu_decompression);
MK_REQUIRE(!result.exposed_native_handles);
```

- [x] Add fail-closed tests for incomplete status, failed status, caller-owned RHI resource destination evidence missing, submitted byte mismatch, and duplicate destination rows.
- [x] Add the test target to root `CMakeLists.txt`.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_page_gpu_resource_update_tests
```

Expected: compile failure because `mirakana/runtime_rhi/mavg_page_gpu_resource_update.hpp` and planner symbols do not exist.

## Task 2: GREEN Readiness API And Implementation

- [x] Add `mavg_page_gpu_resource_update.hpp` with the public API shape above.
- [x] Implement `mavg_page_gpu_resource_update.cpp` to validate:
  - non-zero `graph_asset`
  - present successful buffer destination plan
  - present successful dispatch result
  - present status result with `complete=true`, `failed=false`, and `status=complete`
  - dispatch/status ticket match
  - dispatch uses caller-owned RHI DirectStorage resource destination evidence
  - status uses caller-owned RHI DirectStorage resource destination evidence
  - submitted request count matches planned destination count
  - dispatch/status resource destination bytes match plan destination bytes
  - destination rows are valid and unique by page index and mount id
- [x] On success, emit:
  - `RuntimeMavgResidentPageResourceRow` with `RhiResidencyResourceKind::buffer`
  - `RuntimeMavgPageGpuResourceUpdateRow`
  - aggregate ready counts and byte totals
- [x] Keep execution non-claims explicit:
  - `invoked_file_io=false`
  - `allocated_rhi_resources=false`
  - `invoked_rhi_residency_action=false`
  - `invoked_native_make_resident=false`
  - `invoked_native_evict=false`
  - `enforced_allocator_budget=false`
  - `mutated_mount_set=false`
  - `used_gpu_decompression=false`
  - `exposed_native_handles=false`
- [x] Add the source to `engine/runtime_rhi/CMakeLists.txt`.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_page_gpu_resource_update_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_rhi_mavg_page_gpu_resource_update_tests
```

Expected: PASS.

## Task 3: Docs, Manifest, And Static Guard

- [x] Update current capabilities, roadmap, MAVG master plan, and plan registry with the new readiness evidence and non-claims.
- [x] Update `engine/agent/manifest.fragments/004-modules.json` and `010-aiOperableProductionLoop.json`.
- [x] Compose the manifest:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
```

- [x] Add `tools/check-ai-integration-132-mavg-page-gpu-resource-update-readiness.ps1` to assert the new API, test target, docs, manifest evidence, and non-claims.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
```

## Task 4: Validation And Publication

- [x] Run focused related tests:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_page_gpu_resource_update_tests MK_runtime_rhi_mavg_page_gpu_buffer_destination_tests MK_runtime_rhi_mavg_residency_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_rhi_mavg_page_gpu_resource_update_tests|MK_runtime_rhi_mavg_page_gpu_buffer_destination_tests|MK_runtime_rhi_mavg_residency_tests"
```

- [x] Run slice checks:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1
```

- [x] Commit, push, and open a stacked draft PR against `codex/mavg-page-gpu-buffer-destination-plan-v1`.

## Done When

- `MK_runtime_rhi` can convert completed caller-owned DirectStorage/RHI buffer destination evidence into deterministic resident page resource rows.
- Incomplete/failed status, missing resource-destination evidence, byte/count mismatches, and duplicate rows fail before residency action execution.
- Docs, manifest fragments, composed manifest, static guards, focused tests, and full validation match the exact narrow evidence.
- DirectStorage queue submission, GPU decompression, automatic RHI allocation, allocator/GPU budget enforcement, `MakeResident` / `Evict`, mount mutation, package streaming execution, Vulkan/Metal native IO parity, mesh shaders, Nanite compatibility/equivalence/superiority, async-overlap/performance, and broad optimization remain unclaimed.

## Validation Evidence

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_page_gpu_resource_update_tests` failed as expected with missing `mirakana/runtime_rhi/mavg_page_gpu_resource_update.hpp`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_page_gpu_resource_update_tests` passed.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_rhi_mavg_page_gpu_resource_update_tests` passed.
- Agent surface: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` passed.
- Static: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after making the test-name needle independent of formatter line wrapping.
- Static: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed.
- Static: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed.
- Static: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed after `tools/format.ps1`.
- Focused related build: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_page_gpu_resource_update_tests MK_runtime_rhi_mavg_page_gpu_buffer_destination_tests MK_runtime_rhi_mavg_residency_tests` passed.
- Focused related tests: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_rhi_mavg_page_gpu_resource_update_tests|MK_runtime_rhi_mavg_page_gpu_buffer_destination_tests|MK_runtime_rhi_mavg_residency_tests"` passed, 3/3 tests.
- Full validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed, including 112/112 CTest tests. Metal/Apple checks remain host-gated or diagnostic-only on this Windows host, as expected.
- Publication preflight: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1` passed.
- Publication: stacked draft PR #505 opened against `codex/mavg-page-gpu-buffer-destination-plan-v1`.
- PR evidence sync: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed after PR #505 evidence was added to the plan registry, MAVG master plan, and manifest fragments.
