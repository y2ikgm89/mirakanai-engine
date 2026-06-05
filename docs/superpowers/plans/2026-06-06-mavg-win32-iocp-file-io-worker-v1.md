# MAVG Win32 IOCP File IO Worker v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` or focused inline TDD execution to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `mavg-win32-iocp-file-io-worker-v1`

**Status:** Active.

Focused child over `mavg-runtime-lod-milestone-v1`, stacked after draft PR #463 (`mavg-win32-async-file-io-adapter-v1`).

**Goal:** Add a concrete Windows IO completion port worker adapter for MAVG payload page requests so successful DirectStorage-shaped file-to-memory requests can complete through an engine-owned Win32 background worker without adding the DirectStorage SDK or exposing native handles.

**Architecture:** Keep `MK_runtime` handle-free and unchanged by implementing a second `MK_runtime_host_win32` adapter behind the existing `IRuntimeMavgPayloadNativeIoDispatcher` interface. The new adapter shares the same request rows and caller-owned destination-memory contract as the caller-polled Win32 adapter, but associates file handles with a private IO completion port and records completion on a private worker thread.

**Tech Stack:** C++23, `MK_runtime`, `MK_runtime_host_win32`, Win32 `CreateIoCompletionPort` / `GetQueuedCompletionStatus` / `PostQueuedCompletionStatus` / `CreateFileW` / `ReadFile` / `CancelIoEx`, Microsoft Learn official docs, Context7 Microsoft Learn corpus, PowerShell validation scripts.

---

## Official Source Audit

Checked on 2026-06-06:

- Context7 resolved to the broad Microsoft Learn corpus, but the IOCP query returned unrelated managed-threading snippets. Microsoft Learn pages below are the authoritative implementation constraints for this code.
- Microsoft Learn I/O Completion Ports (`https://learn.microsoft.com/en-us/windows/win32/fileio/i-o-completion-ports`) states that IOCP is intended for efficient processing of many asynchronous I/O requests, that handles are associated through `CreateIoCompletionPort`, that workers wait with `GetQueuedCompletionStatus`, and that `PostQueuedCompletionStatus` can notify worker threads.
- Microsoft Learn `CreateIoCompletionPort` (`https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-createiocompletionport`) requires overlapped file handles, commonly opened with `CreateFile` and `FILE_FLAG_OVERLAPPED`, and allows multiple handles to share one completion port.
- Microsoft Learn `GetQueuedCompletionStatus` (`https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-getqueuedcompletionstatus`) returns the transferred byte count, completion key, and `OVERLAPPED` pointer for dequeued completion packets, and failure with a non-null overlapped pointer represents a failed operation packet.
- Microsoft Learn `PostQueuedCompletionStatus` (`https://learn.microsoft.com/en-us/windows/win32/fileio/postqueuedcompletionstatus`) is the official way to post an application-defined packet to a completion port, used here only to wake and shut down the private worker.
- Microsoft Learn Synchronization and Overlapped Input and Output (`https://learn.microsoft.com/en-us/windows/win32/sync/synchronization-and-overlapped-input-and-output`) requires initialized `OVERLAPPED` storage and keeping buffers valid until completion.
- Microsoft Learn Canceling Pending I/O Operations (`https://learn.microsoft.com/en-us/windows/win32/fileio/canceling-pending-i-o-operations`) states cancellation is not guaranteed and overlapped storage must not be reused until the operation completes or is canceled.
- DirectStorage SDK native queue execution remains deferred: this host lacks `dstorage.h` under Windows Kits and the repo has no DirectStorage SDK dependency. The later native DirectStorage candidate must start with dependency/legal/tooling policy for Microsoft.Direct3D.DirectStorage NuGet or another accepted official acquisition path.

## Scope

In scope:

- Add `Win32MavgPayloadIocpFileIoDispatcherDesc` and `Win32MavgPayloadIocpFileIoDispatcher` to the existing Win32 MAVG payload IO public header.
- Keep the public header free of `windows.h`, `HANDLE`, `OVERLAPPED`, `IDStorage*`, `ID3D12*`, and RHI/native handles.
- Implement one private IO completion port and at least one private worker thread in `engine/runtime_host/win32/src/win32_mavg_payload_io.cpp`.
- Open source files with `CreateFileW(... FILE_FLAG_OVERLAPPED ...)`, associate each handle with the private IOCP, enqueue `ReadFile` operations, and complete status by processing `GetQueuedCompletionStatus` packets.
- Return `executed_background_worker = true` for successful IOCP-worker dispatch/status rows, while keeping `used_win32_async_io = true` and `used_native_directstorage = false`.
- Prove multiple planned byte ranges can be copied into caller-owned memory by the IOCP worker.
- Prove invalid destination ranges fail before a ticket and before destination memory is modified.
- Update docs, manifest fragments, composed manifest, and static checks so the active pointer selects this child and #463 evidence becomes completed/retained.

Out of scope:

- DirectStorage SDK dependency, `dstorage.h` compilation, `IDStorageFactory`, `IDStorageQueue`, `IDStorageStatusArray`, `IDStorageQueue3`, or `ID3D12Fence`.
- Replacing the caller-polled `Win32MavgPayloadAsyncFileIoDispatcher`.
- Automatic eviction policy, resident mount mutation, package streaming execution, GPU memory pressure integration, renderer/RHI handle access, Vulkan/Metal native IO parity, mesh shaders, deformation, ray tracing, Nanite compatibility/equivalence/superiority, benchmark superiority, or broad async-overlap/performance claims.

## Files

- Modify: `engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_mavg_payload_io.hpp`
- Modify: `engine/runtime_host/win32/src/win32_mavg_payload_io.cpp`
- Modify: `tests/unit/runtime_host_win32_tests.cpp`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md`
- Modify: `docs/superpowers/plans/2026-06-06-mavg-win32-async-file-io-adapter-v1.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Compose: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration-116-mavg-win32-async-file-io-adapter.ps1`
- Add: `tools/check-ai-integration-117-mavg-win32-iocp-file-io-worker.ps1`
- Modify if needed: `tools/check-json-contracts-030-tooling-contracts.ps1`
- Modify if needed: `tools/check-json-contracts-040-agent-surfaces.ps1`

## Public API Contract

Add a sibling Win32 host adapter; do not change `MK_runtime` request/status types for this slice:

```cpp
struct Win32MavgPayloadIocpFileIoDispatcherDesc {
    std::filesystem::path root_path;
    std::size_t max_inflight_submissions{64};
    std::size_t worker_thread_count{1};
};

class Win32MavgPayloadIocpFileIoDispatcher final : public runtime::IRuntimeMavgPayloadNativeIoDispatcher {
  public:
    explicit Win32MavgPayloadIocpFileIoDispatcher(Win32MavgPayloadIocpFileIoDispatcherDesc desc);
    ~Win32MavgPayloadIocpFileIoDispatcher() override;

    Win32MavgPayloadIocpFileIoDispatcher(const Win32MavgPayloadIocpFileIoDispatcher&) = delete;
    Win32MavgPayloadIocpFileIoDispatcher& operator=(const Win32MavgPayloadIocpFileIoDispatcher&) = delete;
    Win32MavgPayloadIocpFileIoDispatcher(Win32MavgPayloadIocpFileIoDispatcher&&) noexcept;
    Win32MavgPayloadIocpFileIoDispatcher& operator=(Win32MavgPayloadIocpFileIoDispatcher&&) noexcept;

    [[nodiscard]] runtime::RuntimeMavgPayloadNativeIoBackend backend() const noexcept override;
    [[nodiscard]] runtime::RuntimeMavgPayloadNativeIoDispatchBackendResult
    dispatch(std::span<const runtime::RuntimeMavgPayloadDirectStorageRequestRow> requests,
             const runtime::RuntimeMavgPayloadNativeIoDispatchBackendDesc& desc) override;
    [[nodiscard]] runtime::RuntimeMavgPayloadNativeIoStatusBackendResult poll_status(std::uint64_t ticket) override;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
```

## Tasks

### Task 1: Add Failing IOCP Worker Tests

**Files:**

- Modify: `tests/unit/runtime_host_win32_tests.cpp`

- [x] Add a helper overload that can poll any `runtime::IRuntimeMavgPayloadNativeIoDispatcher&` until a ticket completes or fails.
- [x] Add a test named `win32 mavg payload iocp file io worker reads multiple planned byte ranges into destination memory` that writes `payload.bin`, builds two `RuntimeMavgPayloadDirectStorageRequestRow` values, dispatches through `Win32MavgPayloadIocpFileIoDispatcher`, verifies `dispatch.executed_background_worker`, polls to completion, verifies `status.executed_background_worker`, and verifies only the requested destination byte ranges changed.
- [x] Add a test named `win32 mavg payload iocp file io worker rejects destination overflow before ticket` that uses an invalid destination range and proves `dispatch.ticket == 0`, `!dispatch.submitted_io_queue`, `!dispatch.executed_background_worker`, and unchanged destination bytes.
- [x] Run RED:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset desktop-runtime --target MK_runtime_host_win32_tests
```

Expected: the build fails because `Win32MavgPayloadIocpFileIoDispatcher` is not declared yet.

Evidence: RED observed on 2026-06-06. `MK_runtime_host_win32_tests` failed to build because `Win32MavgPayloadIocpFileIoDispatcher` and `Win32MavgPayloadIocpFileIoDispatcherDesc` were not declared.

### Task 2: Implement The Public IOCP Worker Adapter Surface

**Files:**

- Modify: `engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_mavg_payload_io.hpp`

- [x] Add `Win32MavgPayloadIocpFileIoDispatcherDesc` and `Win32MavgPayloadIocpFileIoDispatcher` exactly as specified in the public API contract.
- [x] Keep the header PIMPL-only and verify these forbidden strings are absent from the public header: `HANDLE`, `OVERLAPPED`, `windows.h`, `IDStorage`, `ID3D12`.
- [x] Run the same RED build again; expected failure moves from missing class to missing implementation symbols:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset desktop-runtime --target MK_runtime_host_win32_tests
```

Evidence: RED moved on 2026-06-06 from missing type declarations to linker errors for `Win32MavgPayloadIocpFileIoDispatcher` constructor/destructor.

### Task 3: Implement The Private IOCP Worker

**Files:**

- Modify: `engine/runtime_host/win32/src/win32_mavg_payload_io.cpp`

- [x] Add private RAII state for a completion port, `std::thread` worker rows, mutex-protected submissions, per-request `OVERLAPPED` storage, and completion diagnostics. Use stable per-read heap storage so an `OVERLAPPED*` received from IOCP is valid until the operation completes or is canceled.
- [x] In `dispatch`, validate `root_path`, submission limit, destination memory, file-to-memory request shape, relative single-line source path, and destination range before submitting any read.
- [x] Open each file with `CreateFileW` and `FILE_FLAG_OVERLAPPED`, associate it with the completion port through `CreateIoCompletionPort`, then issue `ReadFile` using the caller-owned destination subspan.
- [x] Treat `ReadFile == FALSE && GetLastError() == ERROR_IO_PENDING` as a queued request. Treat `ReadFile == FALSE` with any other error as a dispatch failure row and cancel already-submitted reads without destroying live `OVERLAPPED` storage.
- [x] Implement worker loop with `GetQueuedCompletionStatus`; update the owning submission for success, failed packets, short reads, and shutdown packets posted through `PostQueuedCompletionStatus`.
- [x] Implement destructor cleanup with `CancelIoEx`, shutdown posts, thread joins, and handle closure. Do not claim cancellation is guaranteed.
- [x] Keep `max_inflight_submissions` deterministic under concurrent `dispatch` by reserving the submission slot and ticket atomically under the dispatcher mutex.
- [x] Release any not-returned ticket/submission on dispatch failure after IO has begun by canceling submitted reads, waiting for completion packets, and erasing the submission before returning `ticket == 0`.
- [x] Bound `worker_thread_count` and safely post shutdown/join already-started worker threads if constructor thread startup throws before the `Impl` destructor can run.
- [x] Add regression tests named `win32 mavg payload iocp file io worker releases inflight slot after read failure` and `win32 mavg payload iocp file io worker keeps concurrent dispatch under inflight limit`.
- [x] Return these evidence flags for successful IOCP-worker dispatch/status:

```cpp
used_native_directstorage = false;
used_win32_async_io = true;
executed_background_worker = true;
touched_renderer_or_rhi_handles = false;
```

- [x] Run GREEN:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset desktop-runtime --target MK_runtime_host_win32_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset desktop-runtime --output-on-failure -R "MK_runtime_host_win32_tests"
```

Evidence: GREEN passed on 2026-06-06 with `tools/cmake.ps1 --build --preset desktop-runtime --target MK_runtime_host_win32_tests` and `tools/ctest.ps1 --preset desktop-runtime --output-on-failure -R "MK_runtime_host_win32_tests"`. Review-driven RED was also observed on 2026-06-06: `MK_runtime_host_win32_tests` failed before the fix because concurrent dispatch exceeded `max_inflight_submissions`; after the atomic reservation/release fix, the same build and CTest lane passed.

### Task 4: Sync Docs, Manifest, Static Checks, And Validate

**Files:** current capabilities, roadmap, MAVG architecture spec, master plan, parent/child plans, manifest fragments, composed manifest, and static checks.

- [x] Mark `mavg-win32-async-file-io-adapter-v1` completed/published through draft PR #463.
- [x] Select `mavg-win32-iocp-file-io-worker-v1` as the active child.
- [x] Record only Win32 IOCP worker readiness; keep DirectStorage SDK/native queue execution, automatic eviction, GPU memory pressure, mesh shaders, Nanite, and performance superiority unclaimed.
- [x] Compose manifest and run focused validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Preset desktop-runtime -Files engine/runtime_host/win32/src/win32_mavg_payload_io.cpp,tests/unit/runtime_host_win32_tests.cpp
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
git diff --check
```

Evidence: Passed on 2026-06-06: `tools/compose-agent-manifest.ps1 -Write`, `tools/check-public-api-boundaries.ps1`, `tools/check-tidy.ps1 -Preset desktop-runtime -Files engine/runtime_host/win32/src/win32_mavg_payload_io.cpp,tests/unit/runtime_host_win32_tests.cpp`, `tools/check-ai-integration.ps1`, `tools/check-json-contracts.ps1`, `tools/check-agents.ps1`, `tools/check-format.ps1`, and `git diff --check`.

- [x] Run full slice validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Evidence: Passed on 2026-06-06. `tools/validate.ps1` completed with static checks, build, tidy smoke, and 109/109 CTest tests passing. Host-gated Apple/Metal checks remained diagnostic-only on Windows.

### Task 5: Publish Candidate

**Files:** Git/GitHub only.

- [ ] Run publication preflight:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1
```

- [ ] Commit validated candidate:

```powershell
git add <task-owned files>
git commit -m "Add MAVG Win32 IOCP file IO worker"
```

- [ ] Push and create a draft PR with base `codex/mavg-win32-async-file-io-adapter-v1` and head `codex/mavg-win32-iocp-file-io-worker-v1`.

## Done When

- The IOCP worker adapter is visible as a PIMPL-backed public Win32 host class with no native handle exposure.
- A Windows host test proves multiple planned file byte ranges are copied into caller-owned destination memory through private IOCP worker completion.
- Invalid destination ranges fail before a successful ticket and before destination memory mutation.
- Dispatch/status rows set `executed_background_worker = true` only for the IOCP worker adapter and keep `used_native_directstorage = false`.
- Docs, plans, manifest fragments, composed manifest, and static checks describe exactly the implemented IOCP worker scope.
- Full `tools/validate.ps1` passes or records a concrete host/tool blocker.

## Non-Claims

- No DirectStorage SDK dependency, `dstorage.h` compile path, `IDStorageFactory`, `IDStorageQueue`, `IDStorageStatusArray`, `IDStorageQueue3`, or `ID3D12Fence` execution.
- No automatic eviction policy, GPU memory pressure integration, resident mount mutation, or package streaming execution.
- No renderer/RHI/native handle exposure.
- No Vulkan/Metal native IO execution.
- No async-overlap/performance benchmark claim, Nanite compatibility/equivalence/superiority, or benchmark superiority claim.
