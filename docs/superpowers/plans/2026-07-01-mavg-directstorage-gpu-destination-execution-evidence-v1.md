# MAVG DirectStorage GPU Destination Execution Evidence v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a first-party value-only DirectStorage 1.3 GPU destination execution evidence gate that can promote reviewed D3D12 GPU destination rows while preserving GDeflate, Zstd preview, performance, package-visible backend, Nanite, and external-engine non-claims.

**Architecture:** The new gate lives in `MK_runtime_rhi` beside the existing policy-only DirectStorage/GDeflate gate. It accepts retained evidence rows with official-source ids, SDK version, D3D12 queue/device/fence/resource-state/readback/package-output proof flags, and artifact hashes; it exposes no native DirectStorage or D3D12 handles. The default validator remains host-gated until exact retained host artifacts are supplied, but unit tests prove the pure value contract can accept a reviewed row and fail closed on unsafe claims.

**Tech Stack:** C++23, CMake `dev` preset, first-party test framework, PowerShell 7 validators/static guards, JSON Schema draft 2020-12, Microsoft official DirectStorage 1.3 documentation, DirectStorage 1.4 preview documentation, and D3D12 synchronization/resource-state guidance.

---

## File Map

- Create `engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_directstorage_gpu_destination_execution.hpp`: public value-only evidence API, enums, diagnostics, row/result structs, evaluator helpers.
- Create `engine/runtime_rhi/src/mavg_directstorage_gpu_destination_execution.cpp`: fail-closed validation implementation.
- Create `tests/unit/runtime_rhi_mavg_directstorage_gpu_destination_execution_tests.cpp`: focused contract tests for ready rows and blocked/host-gated cases.
- Modify `engine/runtime_rhi/CMakeLists.txt`: register the new source in `mirakana_runtime_rhi`.
- Modify `CMakeLists.txt`: add `MK_runtime_rhi_mavg_ds_gpu_destination_execution_tests`.
- Create `schemas/mavg-directstorage-gpu-destination-execution.schema.json`: retained evidence artifact contract.
- Create `tools/validate-mavg-directstorage-gpu-destination-execution.ps1`: focused validator and default host-gated evidence counters.
- Create `tools/check-ai-integration-156-mavg-directstorage-gpu-destination-execution.ps1`: agent-surface/static guard.
- Modify `docs/current-capabilities.md`, `docs/roadmap.md`, `docs/superpowers/plans/README.md`, and `docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md`: retain exact counters and non-claims.
- Modify `engine/agent/manifest.fragments/002-commands.json`, `004-modules.json`, and `010-aiOperableProductionLoop.json`, then compose `engine/agent/manifest.json`.

## Official Source Decisions

- DirectStorage 1.3 is the selected stable API row because Microsoft documents `IDStorageQueue3::EnqueueRequests` synchronized with an `ID3D12Fence` and `DSTORAGE_DESTINATION_MULTIPLE_SUBRESOURCES_RANGE` for writing contiguous subresources/mips into a D3D12 resource.
- DirectStorage 1.4/Zstd/GACL remains preview/review-required and cannot promote this gate. It requires explicitly authored Zstd/GACL assets and explicit API usage in a later slice.
- GDeflate remains execution-unready in this slice. The gate may prove uncompressed GPU destination execution only; any GDeflate/Zstd execution claim is blocked.
- Legal/originality boundary: no Unity, Unreal, Godot, or Nanite code, samples, UI, schemas, assets, trademarks, compatibility, parity, replacement, superiority, or endorsement is added or claimed.

## Task 1: TDD Red For Runtime RHI Evidence Contract

**Files:**
- Create: `tests/unit/runtime_rhi_mavg_directstorage_gpu_destination_execution_tests.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add the focused test target**

Add this target after `MK_runtime_rhi_mavg_ds_gpu_decomp_policy_tests` in `CMakeLists.txt`:

```cmake
    add_executable(MK_runtime_rhi_mavg_ds_gpu_destination_execution_tests
        tests/unit/runtime_rhi_mavg_directstorage_gpu_destination_execution_tests.cpp
    )
    target_link_libraries(MK_runtime_rhi_mavg_ds_gpu_destination_execution_tests PRIVATE MK_runtime_rhi)
    target_include_directories(MK_runtime_rhi_mavg_ds_gpu_destination_execution_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_rhi_mavg_ds_gpu_destination_execution_tests)
    add_test(NAME MK_runtime_rhi_mavg_ds_gpu_destination_execution_tests
        COMMAND MK_runtime_rhi_mavg_ds_gpu_destination_execution_tests
    )
```

- [ ] **Step 2: Write failing tests**

Create `tests/unit/runtime_rhi_mavg_directstorage_gpu_destination_execution_tests.cpp` with tests that include `mirakana/runtime_rhi/mavg_directstorage_gpu_destination_execution.hpp`, build a reviewed `d3d12_multiple_subresources_range` row, and assert:

```cpp
MK_REQUIRE(result.status == RuntimeMavgDirectStorageGpuDestinationExecutionStatus::ready);
MK_REQUIRE(result.mavg_directstorage_gpu_destination_execution_ready);
MK_REQUIRE(result.mavg_directstorage_multiple_subresources_range_execution_ready);
MK_REQUIRE(!result.mavg_directstorage_gdeflate_execution_ready);
MK_REQUIRE(!result.mavg_directstorage_zstd_preview_ready);
MK_REQUIRE(!result.mavg_directstorage_performance_ready);
MK_REQUIRE(!result.mavg_package_visible_backend_readiness_ready);
MK_REQUIRE(!result.mavg_broad_cpu_gpu_memory_optimization_ready);
MK_REQUIRE(!result.mavg_nanite_compatible);
MK_REQUIRE(!result.mavg_nanite_equivalent);
MK_REQUIRE(!result.mavg_nanite_superior);
MK_REQUIRE(!result.mavg_external_engine_compatibility);
```

Also add tests for missing queue/fence/readback evidence, invalid subresource range, DirectStorage 1.4 preview selection, native handle exposure, GDeflate/Zstd/performance/package/broad/Nanite/external-engine claims, and a buffer destination row that promotes GPU destination execution without promoting multiple-subresource-range execution.

- [ ] **Step 3: Run red build**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_ds_gpu_destination_execution_tests
```

Expected: FAIL because `mavg_directstorage_gpu_destination_execution.hpp` does not exist yet.

## Task 2: Runtime RHI Value API And Implementation

**Files:**
- Create: `engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_directstorage_gpu_destination_execution.hpp`
- Create: `engine/runtime_rhi/src/mavg_directstorage_gpu_destination_execution.cpp`
- Modify: `engine/runtime_rhi/CMakeLists.txt`

- [ ] **Step 1: Add the public header**

Create the header with these public names:

```cpp
enum class RuntimeMavgDirectStorageGpuDestinationKind : std::uint8_t {
    d3d12_buffer,
    d3d12_texture_region,
    d3d12_multiple_subresources_range,
};

enum class RuntimeMavgDirectStorageGpuDestinationExecutionStatus : std::uint8_t {
    host_evidence_required,
    blocked,
    ready,
};

struct RuntimeMavgDirectStorageGpuDestinationExecutionRow {
    RuntimeMavgDirectStorageGpuDestinationKind destination{RuntimeMavgDirectStorageGpuDestinationKind::d3d12_buffer};
    std::string_view row_id;
    std::string_view official_source_id;
    std::string_view sdk_package_version;
    std::string_view retained_artifact_id;
    std::string_view retained_artifact_sha256;
    std::uint64_t requested_bytes{0};
    std::uint64_t completed_bytes{0};
    std::uint32_t first_subresource{0};
    std::uint32_t num_subresources{0};
    bool reviewed{false};
    bool ready{false};
    bool stable_directstorage_13_selected{false};
    bool directstorage_14_preview_selected{false};
    bool queue_device_bound{false};
    bool destination_resource_device_matches_queue_device{false};
    bool enqueue_requests_used{false};
    bool d3d12_fence_synchronization_used{false};
    bool destination_resource_state_common{false};
    bool request_completed{false};
    bool status_array_success{false};
    bool readback_hash_ready{false};
    bool package_visible_output_ready{false};
    bool native_handles_exposed{false};
    bool claims_gdeflate_execution_ready{false};
    bool claims_zstd_preview_ready{false};
    bool claims_performance_gain{false};
    bool claims_package_visible_backend_readiness{false};
    bool claims_broad_mavg_backend_ready{false};
    bool claims_broad_optimization_ready{false};
    bool claims_nanite_compatibility{false};
    bool claims_nanite_equivalence{false};
    bool claims_nanite_superiority{false};
    bool claims_unity_unreal_godot_compatibility{false};
};
```

The result struct must expose counters and booleans for GPU destination execution, multiple-subresource-range execution, GDeflate, Zstd preview, performance, package-visible backend readiness, broad optimization, Nanite, external-engine compatibility, and native-handle exposure.

- [ ] **Step 2: Implement fail-closed validation**

Implement `evaluate_runtime_mavg_directstorage_gpu_destination_execution_evidence` so a row is ready only when every required official-source, SDK, artifact, queue/device, `EnqueueRequests`, D3D12 fence, `D3D12_RESOURCE_STATE_COMMON`, completion, status array success, readback hash, package output, and byte-count field is valid. `d3d12_multiple_subresources_range` additionally requires `num_subresources > 0`.

Add diagnostics for each rejected condition, including `missing_official_source`, `missing_sdk_version`, `missing_stable_directstorage_13`, `directstorage_preview_selected`, `missing_queue_device_binding`, `destination_device_mismatch`, `missing_enqueue_requests`, `missing_d3d12_fence_synchronization`, `missing_destination_state_common`, `missing_completed_request`, `missing_status_success`, `missing_readback_hash`, `missing_package_visible_output`, `invalid_subresource_range`, `invalid_retained_artifact_hash`, `native_handle_exposure`, `gdeflate_execution_claim_not_allowed`, `zstd_preview_claim_not_allowed`, `performance_claim_not_allowed`, `package_backend_readiness_claim_not_allowed`, `broad_backend_claim_not_allowed`, `broad_optimization_claim_not_allowed`, `nanite_claim_not_allowed`, and `external_engine_claim_not_allowed`.

- [ ] **Step 3: Register the source**

Add `src/mavg_directstorage_gpu_destination_execution.cpp` to `engine/runtime_rhi/CMakeLists.txt`.

- [ ] **Step 4: Run focused tests**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_ds_gpu_destination_execution_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_rhi_mavg_ds_gpu_destination_execution_tests
```

Expected: PASS.

## Task 3: Schema, Validator, Static Guard

**Files:**
- Create: `schemas/mavg-directstorage-gpu-destination-execution.schema.json`
- Create: `tools/validate-mavg-directstorage-gpu-destination-execution.ps1`
- Create: `tools/check-ai-integration-156-mavg-directstorage-gpu-destination-execution.ps1`

- [ ] **Step 1: Add the schema**

Create `GameEngine.MavgDirectStorageGpuDestinationExecutionEvidence.v1` with `planId` const `mavg-directstorage-gpu-destination-execution-evidence-v1`, evidence rows, official source ids, SHA-256 artifact hashes, destination values, and summary counters. The summary must allow `mavg_directstorage_gpu_destination_execution_ready` and `mavg_directstorage_multiple_subresources_range_execution_ready` to be `0` or `1`, but must keep `mavg_directstorage_gdeflate_execution_ready`, `mavg_directstorage_zstd_preview_ready`, `mavg_directstorage_performance_ready`, `mavg_package_visible_backend_readiness_ready`, `mavg_broad_cpu_gpu_memory_optimization_ready`, `mavg_nanite_compatible`, `mavg_nanite_equivalent`, `mavg_nanite_superior`, and `mavg_external_engine_compatibility` at `0`.

- [ ] **Step 2: Add the validator**

Create `tools/validate-mavg-directstorage-gpu-destination-execution.ps1` that configures `dev`, builds/runs `MK_runtime_rhi_mavg_ds_gpu_destination_execution_tests`, checks schema/docs/manifest needles, and emits these default host-gated counters:

```text
validation_recipe=mavg-directstorage-gpu-destination-execution-evidence
mavg_directstorage_gpu_destination_execution_status=host_evidence_required
mavg_directstorage_gpu_destination_execution_ready=0
mavg_directstorage_multiple_subresources_range_execution_ready=0
mavg_directstorage_gdeflate_execution_ready=0
mavg_directstorage_zstd_preview_ready=0
mavg_directstorage_native_handles_exposed=0
mavg_directstorage_performance_ready=0
mavg_package_visible_backend_readiness_ready=0
mavg_broad_cpu_gpu_memory_optimization_ready=0
mavg_nanite_compatible=0
mavg_nanite_equivalent=0
mavg_nanite_superior=0
mavg_external_engine_compatibility=0
```

If `-RequireReady` is supplied without retained host artifacts, the script must fail with a message explaining that retained host evidence is required.

- [ ] **Step 3: Add the static guard**

Create chapter `156` that asserts the public API names, forbidden native-handle tokens in the public header, schema literals, validator counters, docs/manifest counters, and forbidden ready claims for GDeflate, Zstd preview, performance, package backend readiness, Nanite, and external-engine compatibility.

- [ ] **Step 4: Run focused guard checks**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-mavg-directstorage-gpu-destination-execution.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
```

Expected: validator emits host-gated default counters; static guard passes after Task 4 docs/manifest sync.

## Task 4: Docs, Manifest, And Compose

**Files:**
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md`
- Modify: `engine/agent/manifest.fragments/002-commands.json`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Generated by compose: `engine/agent/manifest.json`

- [ ] **Step 1: Update human docs**

Add a retained evidence paragraph naming `mavg-directstorage-gpu-destination-execution-evidence-v1`, `mavg_directstorage_gpu_destination_execution.hpp`, `RuntimeMavgDirectStorageGpuDestinationExecutionRow`, `RuntimeMavgDirectStorageGpuDestinationExecutionResult`, `evaluate_runtime_mavg_directstorage_gpu_destination_execution_evidence`, `GameEngine.MavgDirectStorageGpuDestinationExecutionEvidence.v1`, and `tools/validate-mavg-directstorage-gpu-destination-execution.ps1`.

The docs must record that default validation emits `mavg_directstorage_gpu_destination_execution_ready=0` until retained host artifacts exist, while the value API accepts exact reviewed rows. They must keep GDeflate, Zstd preview, performance, package-visible backend readiness, broad optimization, Nanite, and Unity/Unreal/Godot compatibility at zero/non-claim.

- [ ] **Step 2: Update manifest fragments**

Add command `mavgDirectStorageGpuDestinationExecutionCheck` in `002-commands.json`. Add the new header/recent evidence/purpose text to the `MK_runtime_rhi` module in `004-modules.json`. Add retained evidence text `mavgDirectStorageGpuDestinationExecutionEvidence` to `010-aiOperableProductionLoop.json`.

- [ ] **Step 3: Compose manifest**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
```

Expected: `engine/agent/manifest.json` updates only through the compose output.

## Task 5: Slice Validation And Publication

**Files:** all task-owned files above.

- [ ] **Step 1: Run focused validation**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-mavg-directstorage-gpu-destination-execution.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
```

Expected: all pass; validator emits host-gated default counters.

- [ ] **Step 2: Run full validation**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Expected: PASS.

- [ ] **Step 3: Publish the candidate**

Run publication preflight, commit only task-owned files, push `codex/mavg-directstorage-gpu-destination-evidence`, open a draft PR, wait for CI, convert ready with `tools/ready-task-pr.ps1`, then register auto-merge with `gh pr merge --auto --merge --match-head-commit <headRefOid>`.

Expected: PR Gate succeeds and the merge commit reaches `origin/main`.

## Self-Review

- Spec coverage: the plan covers public API, tests, schema, validator, static guard, docs, manifest compose, validation, and GitHub Flow publication.
- Placeholder scan: no `TBD` or unbounded "handle later" steps remain.
- Type consistency: the selected public API root is `RuntimeMavgDirectStorageGpuDestinationExecution*` and the selected evaluator is `evaluate_runtime_mavg_directstorage_gpu_destination_execution_evidence` throughout.
