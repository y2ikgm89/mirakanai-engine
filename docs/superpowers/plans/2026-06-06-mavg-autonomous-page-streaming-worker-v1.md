# 2026-06-06 MAVG Autonomous Page Streaming Worker v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` or focused inline TDD execution to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `mavg-autonomous-page-streaming-worker-v1`

**Status:** Completed stacked child.

Completed child over `mavg-runtime-lod-milestone-v1`, stacked after draft PR #469 (`mavg-directstorage-sdk-dependency-gate-v1`) and published as stacked draft PR #471. This closeout returns the composed manifest to `recommendedNextPlan.id = next-production-gap-selection`.

**Goal:** Add the first engine-owned MAVG page streaming worker execution boundary so already-reviewed `RuntimeMavgPageStreamingDispatchPlan` rows can commit package pages through the existing safe-point drain on a private worker thread without DirectStorage native queue execution, automatic eviction policy, renderer/RHI ownership, or async-overlap/performance claims.

**Architecture:** Keep the worker in `MK_runtime` and keep it handle-free. The dispatch planner gains an `engine_owned_background_worker` mode that still requires safe-point mutation rows. `execute_runtime_mavg_page_streaming_worker` validates a successful engine-owned dispatch plan, starts a private `std::thread`, drains copied dispatch rows through `execute_runtime_mavg_page_streaming_request_safe_point`, joins before returning deterministic evidence, and reports committed/failed/budget-degraded rows. The worker owns execution evidence only; it does not infer eviction policy, execute DirectStorage file IO, touch renderer/RHI handles, or claim async overlap.

**Tech Stack:** C++23, `MK_runtime`, `std::thread`, existing `IFileSystem`, `RuntimeResidentPackageMountSetV2`, `RuntimeResidentCatalogCacheV2`, `RuntimeMavgPageStreamingDispatchPlan`, PowerShell validation wrappers, Context7 `/kitware/cmake`, Microsoft Learn C++ Standard Library thread docs, and existing repository static checks.

---

## Official Source Audit

Checked on 2026-06-06:

- Context7 `/kitware/cmake` confirms explicit executable test targets, `target_link_libraries`, and `add_test` remain the supported CMake/CTest shape. This child reuses the existing `MK_runtime_mavg_page_streaming_tests` target and does not add a new CMake target.
- Microsoft Learn C++ Standard Library `<thread>` and `thread` class pages document `std::thread` / `join` as the standard-library thread execution boundary on MSVC. This child starts one private worker thread and joins it before returning deterministic evidence.
- Existing project policy keeps `MK_runtime` independent from OS APIs, GPU APIs, DirectStorage SDK headers, renderer/RHI handles, and host-native handles.

## Scope

In scope:

- Add `RuntimeMavgPageStreamingDispatchMode::engine_owned_background_worker`.
- Add `RuntimeMavgPageStreamingWorkerDesc`, `RuntimeMavgPageStreamingWorkerResult`, and `execute_runtime_mavg_page_streaming_worker`.
- Preserve existing caller-owned safe-point and caller-owned background queue modes without executing them in the worker.
- Prove engine-owned worker execution commits multiple reviewed MAVG page package rows through the existing safe-point drain.
- Prove caller-owned dispatch plans are rejected before file IO, mount mutation, catalog refresh, or worker execution.
- Prove deterministic `max_worker_rows` degradation.
- Update docs, manifest fragments, composed manifest, and static checks so this child is the active pointer and DirectStorage SDK Dependency Gate v1 is retained as draft PR #469 evidence.

Out of scope:

- DirectStorage native queue/status/fence execution, `DStorageGetFactory`, `IDStorageFactory`, `IDStorageQueue`, `IDStorageStatusArray`, `IDStorageQueue3`, `ID3D12Fence`, or DirectStorage file IO.
- Replacing the Win32 IOCP payload file IO adapter or optional DirectStorage SDK lane.
- Automatic eviction policy, GPU memory pressure integration, renderer/RHI upload/residency, public native handles, Vulkan/Metal native IO parity, mesh shaders, deformation, ray tracing, Nanite compatibility/equivalence/superiority, benchmark superiority, async-overlap/performance, or broad optimization.

## Files

- Modify: `engine/runtime/include/mirakana/runtime/mavg_page_streaming.hpp`
- Modify: `engine/runtime/src/mavg_page_streaming.cpp`
- Modify: `tests/unit/runtime_mavg_page_streaming_tests.cpp`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md`
- Modify: `docs/superpowers/plans/2026-06-06-mavg-directstorage-sdk-dependency-gate-v1.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Compose: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration-118-mavg-directstorage-sdk-dependency-gate.ps1`
- Add: `tools/check-ai-integration-119-mavg-autonomous-page-streaming-worker.ps1`
- Modify if needed: `tools/check-json-contracts-040-agent-surfaces.ps1`

## Tasks

### Task 1: Add Worker API And Tests

- [x] Add engine-owned dispatch mode and worker result/desc rows.
- [x] Add tests proving planner rows for `engine_owned_background_worker`, successful multi-row worker execution, caller-owned plan rejection, and `max_worker_rows` degradation.
- [x] Run focused build/CTest for `MK_runtime_mavg_page_streaming_tests`.

Evidence: Passed on 2026-06-06:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_page_streaming_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_mavg_page_streaming_tests"
```

### Task 2: Sync Docs, Manifest, Static Checks, And Validate

- [x] Mark MAVG DirectStorage SDK Dependency Gate v1 completed/published as draft PR #469 with optional `dstorage` validation still command-policy/bootstrap blocked in this no-approval session.
- [x] Select `mavg-autonomous-page-streaming-worker-v1` as the active child while implementing it, then close it as stacked draft PR #471 and return the manifest to the production selection gate.
- [x] Record only narrow engine-owned page streaming worker execution; keep DirectStorage native queues, automatic eviction policy, GPU memory pressure, renderer/RHI ownership, async-overlap/performance, mesh shaders, Nanite, and broad optimization unclaimed.
- [x] Compose manifest and run focused validation plus full `tools/validate.ps1`.

Evidence so far on 2026-06-06:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/mavg_page_streaming.cpp,tests/unit/runtime_mavg_page_streaming_tests.cpp
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

## Done When

- `MK_runtime` exposes a handle-free engine-owned MAVG page streaming worker execution boundary.
- `MK_runtime_mavg_page_streaming_tests` proves multi-row package page commits execute through a private worker thread and joined deterministic result.
- Caller-owned dispatch plans are rejected before mutation.
- Deterministic worker row budgeting is covered.
- Docs, plans, manifest fragments, composed manifest, and static checks describe exactly this worker scope.
- Full `tools/validate.ps1` passes or records a concrete host/tool blocker.

## Non-Claims

- No DirectStorage native queue/status/fence/request execution.
- No DirectStorage file IO execution.
- No automatic eviction policy or GPU memory pressure integration.
- No renderer/RHI upload/residency or public native handle exposure.
- No async-overlap/performance, benchmark superiority, or broad optimization claim.
- No Vulkan/Metal native IO parity.
- No mesh shader, deformation, ray tracing, Nanite compatibility/equivalence/superiority, or legal freedom-to-operate claim.
