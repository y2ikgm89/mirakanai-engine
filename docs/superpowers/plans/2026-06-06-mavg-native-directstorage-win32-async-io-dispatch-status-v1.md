# MAVG Native DirectStorage/Win32 Async IO Dispatch Status v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` (recommended) or `superpowers:executing-plans` to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `mavg-native-directstorage-win32-async-io-dispatch-status-v1`

**Status:** Completed stacked child.

Focused child over `mavg-runtime-lod-milestone-v1`, stacked after `mavg-directstorage-request-plan-v1`.

**Successor:** `mavg-win32-async-file-io-adapter-v1` adds `RuntimeMavgPayloadDirectStorageRequestRow::source_file_path`, caller-owned `destination_memory`, and `Win32MavgPayloadAsyncFileIoDispatcher` using private Win32 `CreateFileW`, `ReadFile`, `OVERLAPPED`, `GetOverlappedResult`, and `CancelIoEx` handling while keeping DirectStorage SDK, IOCP, async-overlap/performance, and Nanite claims unclaimed.

**Goal:** Add the first runtime MAVG native/async IO dispatch-status boundary so validated DirectStorage-shaped page request plans can be submitted through a caller-owned backend adapter and polled for completion evidence without hard-linking the DirectStorage SDK in default validation.

**Architecture:** Keep `MK_runtime` independent from `dstorage.h`, Win32 handles, D3D12 resources, worker ownership, and renderer/RHI objects. The public contract consumes `RuntimeMavgPayloadDirectStorageRequestPlanResult` rows and a caller-provided `IRuntimeMavgPayloadNativeIoDispatcher`; production adapters for DirectStorage SDK or Win32 overlapped IO remain backend-local follow-ups, while the runtime status lifecycle is tested through a deterministic first-party test dispatcher.

**Tech Stack:** C++23, `MK_runtime`, existing `MK_assets` payload validation, existing `MK_platform` filesystem rows, PowerShell validation scripts, Context7 Microsoft Learn query, Microsoft Learn DirectStorage API reference, and NuGet `Microsoft.Direct3D.DirectStorage` package evidence.

---

## Official Source Audit

Checked on 2026-06-06:

- Context7 `/websites/learn_microsoft_en-us` was queried for DirectStorage `IDStorageFactory`, `IDStorageQueue`, `IDStorageStatusArray`, request enqueue, status, fence, and NuGet setup; the result was too broad, so Microsoft Learn pages below are the authoritative implementation constraints.
- Microsoft Learn `IDStorageFactory` (`https://learn.microsoft.com/en-us/windows/win32/dstorage/dstorage/nn-dstorage-idstoragefactory`) creates queues, status arrays, opens DirectStorage files, and owns global DirectStorage operations.
- Microsoft Learn `IDStorageQueue::EnqueueRequest` (`https://learn.microsoft.com/en-us/windows/win32/dstorage/dstorage/nf-dstorage-idstoragequeue-enqueuerequest`) keeps requests queued until `Submit` or queue auto-submit behavior.
- Microsoft Learn `IDStorageQueue::EnqueueStatus` (`https://learn.microsoft.com/en-us/windows/win32/dstorage/dstorage/nf-dstorage-idstoragequeue-enqueuestatus`) writes completion status after preceding requests complete.
- Microsoft Learn `IDStorageStatusArray` (`https://learn.microsoft.com/en-us/windows/win32/dstorage/dstorage/nn-dstorage-idstoragestatusarray`) reports completion and HRESULT for requests before a status entry, and status entries cannot be reused until complete.
- Microsoft Learn `IDStorageQueue3::EnqueueRequests` (`https://learn.microsoft.com/en-us/windows/win32/dstorage/dstorage/nf-dstorage-idstoragequeue3-enqueurequests`) adds batched request enqueue with optional `ID3D12Fence` synchronization in DirectStorage 1.3-era docs.
- NuGet `Microsoft.Direct3D.DirectStorage` (`https://www.nuget.org/packages/Microsoft.Direct3D.DirectStorage`) current stable package observed as 1.3.0; it includes the DirectStorage SDK and redistributable binaries, with code and binary license files. This candidate does not add that package yet because the current host has no `dstorage.h` and dependency/legal integration needs a dedicated SDK-gated child.

## Scope

In scope:

- Public value/status types for caller-owned native/async IO dispatch.
- A small adapter interface that accepts already validated request rows and returns submission/status rows.
- Fail-closed diagnostics for missing dispatcher, failed request plan, unsupported backend, dispatch failure, unknown ticket, incomplete status, and failed status.
- Deterministic counters proving enqueue/submit/status/fence intent without exposing native handles.
- Unit tests with a first-party fake dispatcher that simulates DirectStorage/Win32 async behavior.
- Docs, registry, manifest fragments, composed manifest, and static checks updated to claim only this adapter/status boundary.

Out of scope:

- Adding `Microsoft.Direct3D.DirectStorage` as a dependency.
- Including or compiling against `dstorage.h`.
- Native `IDStorageFactory`, `IDStorageQueue`, `IDStorageStatusArray`, `ID3D12Fence`, Win32 `OVERLAPPED`, IOCP, or event-handle execution.
- Autonomous background workers, engine-owned thread pools, async-overlap/performance claims, automatic eviction policy, GPU memory pressure integration, renderer/RHI handle access, Vulkan/Metal backend IO, mesh shaders, Nanite equivalence/superiority, or benchmark superiority.

## Files

- Modify: `engine/runtime/include/mirakana/runtime/mavg_payload_pages.hpp`
- Modify: `engine/runtime/src/mavg_payload_pages.cpp`
- Modify: `tests/unit/runtime_mavg_payload_pages_tests.cpp`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `docs/superpowers/plans/2026-06-05-mavg-directstorage-request-plan-v1.md`
- Modify: `docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Compose: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration-010-agent-baseline.ps1`
- Modify: `tools/check-ai-integration-020-engine-manifest.ps1`
- Modify: `tools/check-ai-integration-030-runtime-rendering.ps1`
- Modify: `tools/check-ai-integration-114-mavg-directstorage-request-plan.ps1`
- Create: `tools/check-ai-integration-115-mavg-native-io-status.ps1`
- Modify: `tools/check-json-contracts-030-tooling-contracts.ps1`
- Modify: `tools/check-json-contracts-040-agent-surfaces.ps1`

## Public API Contract

`engine/runtime/include/mirakana/runtime/mavg_payload_pages.hpp` extends the existing DirectStorage request-plan surface with:

```cpp
enum class RuntimeMavgPayloadNativeIoBackend : std::uint8_t {
    directstorage = 0,
    win32_async_file,
    test_adapter,
};

enum class RuntimeMavgPayloadNativeIoStatus : std::uint8_t {
    submitted = 0,
    complete,
    failed,
};

struct RuntimeMavgPayloadNativeIoDispatchDesc {
    IRuntimeMavgPayloadNativeIoDispatcher* dispatcher{nullptr};
    const RuntimeMavgPayloadDirectStorageRequestPlanResult* request_plan{nullptr};
    RuntimeMavgPayloadNativeIoBackend required_backend{RuntimeMavgPayloadNativeIoBackend::directstorage};
    std::uint64_t submission_tag{0};
    bool require_native_directstorage{true};
    bool enqueue_status_after_requests{true};
    bool signal_fence_after_requests{false};
};

struct RuntimeMavgPayloadNativeIoStatusPollDesc {
    IRuntimeMavgPayloadNativeIoDispatcher* dispatcher{nullptr};
    std::uint64_t ticket{0};
};

[[nodiscard]] RuntimeMavgPayloadNativeIoDispatchResult
dispatch_runtime_mavg_payload_native_io_requests(const RuntimeMavgPayloadNativeIoDispatchDesc& desc);

[[nodiscard]] RuntimeMavgPayloadNativeIoStatusPollResult
poll_runtime_mavg_payload_native_io_status(const RuntimeMavgPayloadNativeIoStatusPollDesc& desc);
```

The interface stays first-party and handle-free. DirectStorage and Win32 concrete adapters are future backend-local work.

## Tasks

### Task 1: Add Failing Native IO Dispatch And Status Tests

**Files:**

- Modify: `tests/unit/runtime_mavg_payload_pages_tests.cpp`

- [x] Add tests for successful dispatch using a deterministic fake dispatcher over a valid `RuntimeMavgPayloadDirectStorageRequestPlanResult`.
- [x] Add tests proving failed request plans and missing dispatchers fail before enqueue/submit/status work.
- [x] Add tests proving status polling returns incomplete and then complete rows for the same ticket.
- [x] Add tests proving failed backend status returns diagnostics and no completion claim.
- [x] Run RED:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_payload_pages_tests
```

Evidence: RED failed on 2026-06-06 because `IRuntimeMavgPayloadNativeIoDispatcher`, `RuntimeMavgPayloadNativeIoDiagnostic`, dispatch/status result types, and dispatch/status functions were not defined.

### Task 2: Implement Runtime Native IO Dispatch/Status Boundary

**Files:**

- Modify: `engine/runtime/include/mirakana/runtime/mavg_payload_pages.hpp`
- Modify: `engine/runtime/src/mavg_payload_pages.cpp`
- Modify: `tests/unit/runtime_mavg_payload_pages_tests.cpp`

- [x] Add public enums, diagnostics, dispatch/status row/result types, and `IRuntimeMavgPayloadNativeIoDispatcher`.
- [x] Implement `dispatch_runtime_mavg_payload_native_io_requests` as a validation wrapper over a caller-owned dispatcher.
- [x] Implement `poll_runtime_mavg_payload_native_io_status` as a validation wrapper over a caller-owned dispatcher ticket.
- [x] Keep explicit flags for `used_native_directstorage`, `used_win32_async_io`, `enqueued_native_requests`, `submitted_native_queue`, `enqueued_status_write`, `signaled_native_fence`, `executed_background_worker`, `mutated_mount_set`, and `touched_renderer_or_rhi_handles`.
- [x] Run GREEN:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_payload_pages_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_mavg_payload_pages_tests"
```

Evidence: GREEN passed on 2026-06-06. The targeted build produced `MK_runtime_mavg_payload_pages_tests.exe`, and targeted CTest passed `1/1`.

### Task 3: Sync Docs, Manifest, Static Checks, And Validate

**Files:**

- Modify the docs/manifest/static files listed above.

- [x] Mark `mavg-directstorage-request-plan-v1` as completed stacked child and select `mavg-native-directstorage-win32-async-io-dispatch-status-v1` as active child.
- [x] Describe only caller-owned adapter/status lifecycle readiness; keep SDK installation and concrete DirectStorage/Win32 execution host-gated.
- [x] Run focused static validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/mavg_payload_pages.cpp,tests/unit/runtime_mavg_payload_pages_tests.cpp -ReuseExistingFileApiReply
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
git diff --check
```

Evidence: Focused static validation passed on 2026-06-06 with `tools/compose-agent-manifest.ps1 -Write`, `tools/check-public-api-boundaries.ps1`, targeted `tools/check-tidy.ps1 -Files engine/runtime/src/mavg_payload_pages.cpp,tests/unit/runtime_mavg_payload_pages_tests.cpp -ReuseExistingFileApiReply`, `tools/check-ai-integration.ps1`, `tools/check-json-contracts.ps1`, `tools/check-agents.ps1`, `tools/check-format.ps1`, and `git diff --check`. The targeted build and CTest were rerun after docs/manifest sync; `MK_runtime_mavg_payload_pages_tests` passed `1/1`.

- [x] Run full slice validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Evidence: Full validation passed on 2026-06-06 with `validate: ok`; static checks passed, build succeeded, tidy smoke passed, and CTest reported `109/109` passing.

### Task 4: Publish Candidate

**Files:** Git/GitHub only.

- [x] Run publication preflight:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1
```

Evidence: Passed on 2026-06-06 for branch `codex/mavg-native-directstorage-win32-async-io-status-v1` with remote head present, GitHub network reachable, and `gh` auth ok.

- [x] Commit validated candidate:

```powershell
git add <task-owned files>
git commit -m "Add MAVG native IO status boundary"
```

Evidence: Commit `010768c0 Add MAVG native IO status boundary`.

- [x] Push and create a draft PR with base `codex/mavg-directstorage-request-plan-v1` and head `codex/mavg-native-directstorage-win32-async-io-status-v1`.

Evidence: Pushed branch `codex/mavg-native-directstorage-win32-async-io-status-v1` and opened draft PR `https://github.com/y2ikgm89/mirakanai-engine/pull/462` stacked on PR #460.

## Done When

- Valid request plans can be dispatched through a caller-owned native/async IO adapter and return deterministic submission rows.
- Submission/status polling can distinguish submitted, incomplete, complete, and failed backend status.
- Invalid inputs fail before adapter calls where appropriate.
- The default build does not require `dstorage.h`, NuGet DirectStorage, Win32 handles, D3D12 fences, or native async IO.
- Docs, plans, registry, manifest fragments, composed manifest, and static checks describe exactly the implemented boundary.
- Full `tools/validate.ps1` passes or records a concrete host/tool blocker.

## Non-Claims

- No DirectStorage SDK dependency, `dstorage.h` compile path, `IDStorageFactory`, `IDStorageQueue`, `IDStorageStatusArray`, `IDStorageQueue3`, `ID3D12Fence`, Win32 `OVERLAPPED`, IOCP, or event handle execution.
- No autonomous background worker, engine-owned thread pool, or async-overlap/performance claim.
- No automatic eviction policy or GPU memory pressure integration.
- No renderer/RHI/native handle exposure.
- No Vulkan/Metal native IO execution.
- No mesh shader, deformation, ray tracing, Nanite compatibility/equivalence/superiority, benchmark superiority, or broad optimization claim.
