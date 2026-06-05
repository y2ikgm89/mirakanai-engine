# MAVG Win32 Async File IO Adapter v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` or focused inline TDD execution to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `mavg-win32-async-file-io-adapter-v1`

**Status:** Completed/published as draft PR #463; retained by active `mavg-win32-iocp-file-io-worker-v1`.

Focused child over `mavg-runtime-lod-milestone-v1`, stacked after draft PR #462 (`mavg-native-directstorage-win32-async-io-dispatch-status-v1`).

**Goal:** Add the first concrete Windows overlapped file-read adapter for MAVG payload page requests so a successful DirectStorage-shaped request plan can fill caller-owned memory through Win32 async file IO without adding the DirectStorage SDK or exposing native handles.

**Architecture:** Keep the public runtime dispatch/status contract handle-free and add only the missing source-path and caller-owned destination-memory facts needed by real file IO. Put the Win32 implementation in `MK_runtime_host_win32`, which is already a Windows host layer above `MK_runtime` and `MK_platform_win32`; do not move Win32 APIs into `MK_runtime` or `MK_platform` in a way that reverses dependency direction.

**Tech Stack:** C++23, `MK_runtime`, `MK_runtime_host_win32`, Win32 `CreateFileW` / `ReadFile` / `OVERLAPPED` / `GetOverlappedResult` / `CancelIoEx`, Microsoft Learn official docs, Context7 Win32 corpus, PowerShell validation scripts.

---

## Official Source Audit

Checked on 2026-06-06:

- Context7 resolved Win32 API documentation to `/websites/learn_microsoft_windows_win32`; the targeted query returned unrelated Windows Installer snippets, so Microsoft Learn pages below are the authoritative implementation constraints for the code.
- Microsoft Learn Synchronization and Overlapped Input and Output (`https://learn.microsoft.com/en-us/windows/win32/sync/synchronization-and-overlapped-input-and-output`) requires overlapped handles to be opened with `FILE_FLAG_OVERLAPPED`, a valid initialized `OVERLAPPED` for each operation, completion detection via `GetOverlappedResult` / waits, and `ERROR_IO_PENDING` handling.
- Microsoft Learn `CreateFileW` (`https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew`) documents `FILE_FLAG_OVERLAPPED` as the async-open flag and says the handle's sync/async behavior is determined by this flag.
- Microsoft Learn `ReadFile` (`https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-readfile`) requires a valid unique `OVERLAPPED` for async handles, keeps the output buffer valid until completion, uses `Offset` / `OffsetHigh` for byte offsets, and treats `ERROR_IO_PENDING` as pending asynchronous completion rather than failure.
- Microsoft Learn `GetOverlappedResult` (`https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-getoverlappedresult`) reports transferred bytes, returns `ERROR_IO_INCOMPLETE` when polled with `bWait = FALSE` and the operation is still pending, and recommends event objects when multiple overlapped operations can be in flight.
- Microsoft Learn I/O Completion Ports (`https://learn.microsoft.com/en-us/windows/win32/fileio/i-o-completion-ports`) is deferred for a later IOCP/worker child; this slice uses caller polling and manual-reset events only.
- Microsoft Learn Canceling Pending I/O Operations (`https://learn.microsoft.com/en-us/windows/win32/fileio/canceling-pending-i-o-operations`) states `CancelIoEx` does not guarantee cancellation and `OVERLAPPED` storage must not be reused until the operation has completed or canceled.

## Scope

In scope:

- Add `source_file_path` to MAVG request rows and set it from `RuntimeMavgPayloadDirectStorageRequestPlanDesc::payload_blob_path`.
- Add caller-owned `destination_memory` to runtime native IO dispatch descriptors and forward it to backend adapters.
- Add `Win32MavgPayloadAsyncFileIoDispatcher` in `engine/runtime_host/win32` implementing `IRuntimeMavgPayloadNativeIoDispatcher`.
- Use `CreateFileW(... FILE_FLAG_OVERLAPPED ...)`, one unique `OVERLAPPED` and manual-reset event per request, `ReadFile`, non-blocking `GetOverlappedResult`, and `CancelIoEx` cleanup.
- Prove actual file bytes are copied into the caller-owned destination buffer for a small Windows host test.
- Keep deterministic diagnostics and explicit no-claim flags for DirectStorage SDK, IOCP, background workers, mount mutation, renderer/RHI handles, async-overlap/performance, and Nanite equivalence/superiority.

Out of scope:

- DirectStorage SDK dependency or `dstorage.h` compilation.
- `IDStorageFactory`, `IDStorageQueue`, `IDStorageStatusArray`, `ID3D12Fence`, or native DirectStorage execution.
- IOCP worker pool, autonomous background streaming worker, async-overlap/performance measurement, automatic eviction policy, GPU memory pressure integration, Vulkan/Metal native IO parity, mesh shaders, deformation, ray tracing, or Nanite superiority claims.

## Files

- Modify: `engine/runtime/include/mirakana/runtime/mavg_payload_pages.hpp`
- Modify: `engine/runtime/src/mavg_payload_pages.cpp`
- Modify: `tests/unit/runtime_mavg_payload_pages_tests.cpp`
- Create: `engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_mavg_payload_io.hpp`
- Create: `engine/runtime_host/win32/src/win32_mavg_payload_io.cpp`
- Modify: `engine/runtime_host/win32/CMakeLists.txt`
- Modify: `tests/unit/runtime_host_win32_tests.cpp`
- Modify docs/manifest/static checks that retain the active MAVG pointer and capability claims.

## Public API Contract

Runtime request rows gain the source path required by real file IO:

```cpp
struct RuntimeMavgPayloadDirectStorageRequestRow {
    std::string source_file_path;
};
```

Runtime native IO dispatch descriptors gain a caller-owned destination buffer that must remain valid until status reports completion or failure:

```cpp
struct RuntimeMavgPayloadNativeIoDispatchBackendDesc {
    std::span<std::uint8_t> destination_memory;
};

struct RuntimeMavgPayloadNativeIoDispatchDesc {
    std::span<std::uint8_t> destination_memory;
};
```

Win32 host adapter:

```cpp
class Win32MavgPayloadAsyncFileIoDispatcher final
    : public runtime::IRuntimeMavgPayloadNativeIoDispatcher {
  public:
    explicit Win32MavgPayloadAsyncFileIoDispatcher(Win32MavgPayloadAsyncFileIoDispatcherDesc desc);
    ~Win32MavgPayloadAsyncFileIoDispatcher() override;

    [[nodiscard]] runtime::RuntimeMavgPayloadNativeIoBackend backend() const noexcept override;
    [[nodiscard]] runtime::RuntimeMavgPayloadNativeIoDispatchBackendResult
    dispatch(std::span<const runtime::RuntimeMavgPayloadDirectStorageRequestRow> requests,
             const runtime::RuntimeMavgPayloadNativeIoDispatchBackendDesc& desc) override;
    [[nodiscard]] runtime::RuntimeMavgPayloadNativeIoStatusBackendResult poll_status(std::uint64_t ticket) override;
};
```

## Tasks

### Task 1: Add Failing Runtime Contract Tests

**Files:**

- Modify: `tests/unit/runtime_mavg_payload_pages_tests.cpp`

- [x] Prove request planning copies `payload_blob_path` into every `RuntimeMavgPayloadDirectStorageRequestRow::source_file_path`.
- [x] Prove `dispatch_runtime_mavg_payload_native_io_requests` forwards `destination_memory` to caller-owned dispatchers.
- [x] Run RED:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_payload_pages_tests
```

Expected: fails because `source_file_path` and `destination_memory` do not exist yet.

Evidence: RED observed on 2026-06-06: `MK_runtime_mavg_payload_pages_tests` failed to build because `source_file_path` and `destination_memory` did not exist yet.

### Task 2: Implement Runtime Contract Extension

**Files:**

- Modify: `engine/runtime/include/mirakana/runtime/mavg_payload_pages.hpp`
- Modify: `engine/runtime/src/mavg_payload_pages.cpp`

- [x] Add `source_file_path` to request rows.
- [x] Set `source_file_path = desc.payload_blob_path` in `plan_runtime_mavg_payload_directstorage_requests`.
- [x] Add `destination_memory` to dispatch desc/backend desc and forward it to the dispatcher.
- [x] Run GREEN:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_payload_pages_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_mavg_payload_pages_tests"
```

Evidence: GREEN passed on 2026-06-06 with `tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_payload_pages_tests` and `tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_mavg_payload_pages_tests"`.

### Task 3: Add Failing Win32 Adapter Tests

**Files:**

- Modify: `tests/unit/runtime_host_win32_tests.cpp`

- [x] Add a Windows-only test that writes a tiny payload file, dispatches a one-row `RuntimeMavgPayloadDirectStorageRequestPlanResult` through `Win32MavgPayloadAsyncFileIoDispatcher`, polls until complete, and verifies destination bytes.
- [x] Add a fail-closed test for destination buffer overflow before any successful ticket is returned.
- [x] Run RED:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset desktop-runtime
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset desktop-runtime --target MK_runtime_host_win32_tests
```

Expected: fails because the Win32 MAVG payload IO header/class/source do not exist yet.

Evidence: RED observed on 2026-06-06. The `dev` preset attempt showed `MK_runtime_host_win32_tests` is a `desktop-runtime` target; after `tools/cmake.ps1 --preset desktop-runtime`, the desktop-runtime build reached the expected missing `mirakana/runtime_host/win32/win32_mavg_payload_io.hpp` header error.

### Task 4: Implement Win32 Async File IO Adapter

**Files:**

- Create: `engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_mavg_payload_io.hpp`
- Create: `engine/runtime_host/win32/src/win32_mavg_payload_io.cpp`
- Modify: `engine/runtime_host/win32/CMakeLists.txt`

- [x] Implement a PIMPL-backed dispatcher with no public `HANDLE`, `OVERLAPPED`, event, or Win32 include leakage from the header.
- [x] Open each request source with `CreateFileW` and `FILE_FLAG_OVERLAPPED`.
- [x] Use one unique initialized `OVERLAPPED` plus manual-reset event per request.
- [x] Keep caller-owned destination memory alive by contract; write directly into `destination_memory.subspan(destination_offset, destination_size)`.
- [x] Treat `ReadFile == TRUE` as immediate completion and `ReadFile == FALSE && GetLastError() == ERROR_IO_PENDING` as submitted pending IO.
- [x] Poll with `GetOverlappedResult(..., FALSE)`; return submitted for `ERROR_IO_INCOMPLETE`, complete when all rows finish, and failed diagnostics for any other error.
- [x] Cancel outstanding reads with `CancelIoEx` during cleanup/failure without claiming cancellation is guaranteed.
- [x] Run GREEN:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset desktop-runtime --target MK_runtime_host_win32_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset desktop-runtime --output-on-failure -R "MK_runtime_host_win32_tests"
```

Evidence: GREEN passed on 2026-06-06 with `tools/cmake.ps1 --build --preset desktop-runtime --target MK_runtime_host_win32_tests` and `tools/ctest.ps1 --preset desktop-runtime --output-on-failure -R "MK_runtime_host_win32_tests"`. After cleanup, the desktop-runtime target and CTest were rerun successfully together with the runtime MAVG payload page target and CTest.

### Task 5: Sync Docs, Manifest, Static Checks, And Validate

**Files:** current capabilities, roadmap, MAVG architecture spec, MAVG parent/child plans, manifest fragments, composed manifest, and static check scripts.

- [x] Mark `mavg-native-directstorage-win32-async-io-dispatch-status-v1` completed/published through draft PR #462.
- [x] Select `mavg-win32-async-file-io-adapter-v1` as the active child.
- [x] Record only Win32 overlapped file-read adapter readiness; keep DirectStorage SDK, IOCP workers, async-overlap/performance, automatic eviction, GPU memory pressure, and Nanite claims unclaimed.
- [x] Compose manifest and run focused validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/mavg_payload_pages.cpp,tests/unit/runtime_mavg_payload_pages_tests.cpp -ReuseExistingFileApiReply
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Preset desktop-runtime -Files engine/runtime_host/win32/src/win32_mavg_payload_io.cpp,tests/unit/runtime_host_win32_tests.cpp
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset desktop-runtime --target MK_runtime_host_win32_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset desktop-runtime --output-on-failure -R "MK_runtime_host_win32_tests"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
git diff --check
```

Evidence: Passed on 2026-06-06:

- `tools/compose-agent-manifest.ps1 -Write`
- `tools/check-public-api-boundaries.ps1`
- `tools/check-tidy.ps1 -Files engine/runtime/src/mavg_payload_pages.cpp,tests/unit/runtime_mavg_payload_pages_tests.cpp -ReuseExistingFileApiReply`
- `tools/check-tidy.ps1 -Preset desktop-runtime -Files engine/runtime_host/win32/src/win32_mavg_payload_io.cpp,tests/unit/runtime_host_win32_tests.cpp`
- `tools/cmake.ps1 --build --preset desktop-runtime --target MK_runtime_host_win32_tests`
- `tools/ctest.ps1 --preset desktop-runtime --output-on-failure -R "MK_runtime_host_win32_tests"`
- `tools/check-ai-integration.ps1`
- `tools/check-json-contracts.ps1`
- `tools/check-agents.ps1`
- `tools/check-format.ps1`
- `git diff --check`

- [x] Run full slice validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Evidence: `tools/validate.ps1` passed on 2026-06-06 after code, docs, manifest, and static-check synchronization. Diagnostic-only host gates remained expected for Metal/Apple on Windows.

### Task 6: Publish Candidate

**Files:** Git/GitHub only.

- [x] Run publication preflight:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1
```

Evidence: Passed on 2026-06-06 for branch `codex/mavg-win32-async-file-io-adapter-v1` with remote missing before push, GitHub network reachable, and `gh` auth ok.

- [x] Commit validated candidate:

```powershell
git add <task-owned files>
git commit -m "Add MAVG Win32 async file IO adapter"
```

- [x] Push and create a draft PR with base `codex/mavg-native-directstorage-win32-async-io-status-v1` and head `codex/mavg-win32-async-file-io-adapter-v1`.

Evidence: Created commit `4ca3aea2` (`Add MAVG Win32 async file IO adapter`), pushed `codex/mavg-win32-async-file-io-adapter-v1`, and opened draft PR `https://github.com/y2ikgm89/mirakanai-engine/pull/463` stacked on PR #462.

## Done When

- Request rows carry source file paths and dispatch rows carry caller-owned destination memory.
- A Windows host test proves the Win32 adapter copies selected file bytes into caller-owned destination memory through overlapped `ReadFile`.
- Invalid destination ranges fail before a successful ticket is returned.
- The adapter owns all Win32 handles privately and closes/cancels pending rows safely.
- Docs, plans, manifest fragments, composed manifest, and static checks describe exactly the implemented Win32 overlapped file-read adapter.
- Full `tools/validate.ps1` passes or records a concrete host/tool blocker.

## Non-Claims

- No DirectStorage SDK dependency, `dstorage.h` compile path, `IDStorageFactory`, `IDStorageQueue`, `IDStorageStatusArray`, `IDStorageQueue3`, or `ID3D12Fence` execution.
- No IOCP worker pool, autonomous background worker, engine-owned streaming thread, or async-overlap/performance claim.
- No automatic eviction policy or GPU memory pressure integration.
- No renderer/RHI/native handle exposure.
- No Vulkan/Metal native IO execution.
- No mesh shader, deformation, ray tracing, Nanite compatibility/equivalence/superiority, benchmark superiority, or broad optimization claim.
