# MAVG DirectStorage Native Execution v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add an optional Windows DirectStorage file-to-memory native queue/status execution adapter for MAVG payload requests without making DirectStorage part of the default build or exposing native handles.

**Architecture:** Keep `MK_runtime` and default `MK_runtime_host_win32` builds free of `dstorage.h`, `IDStorage*`, `DSTORAGE_*`, `ID3D12*`, and native handles. Add a clean optional `MK_runtime_host_win32_directstorage` target, enabled only by `MK_ENABLE_DIRECTSTORAGE_SDK`, that implements `IRuntimeMavgPayloadNativeIoDispatcher` through a private DirectStorage factory, queue, file, and status-array PIMPL.

**Tech Stack:** C++23, Windows, Microsoft DirectStorage SDK 1.3.0 through vcpkg `dstorage`, `Microsoft::DirectStorage`, CMake 3.30 presets, PowerShell validation wrappers, Microsoft Learn DirectStorage API reference, Microsoft DirectStorage samples, Context7 Microsoft Learn lookup.

---

## Official Source Audit

- Context7 did not expose a DirectStorage-specific library id; the closest high-reputation match was Microsoft Learn, but its DirectStorage query returned unrelated snippets. Use Microsoft Learn, Microsoft DirectX blog, the Microsoft DirectStorage GitHub repository, and the local official vcpkg port as primary sources for this child.
- Microsoft's DirectStorage SDK landing page lists stable `Microsoft.Direct3D.DirectStorage` 1.3.0 and notes vcpkg installation support. The local official vcpkg port `external/vcpkg/ports/dstorage/vcpkg.json` is also `1.3.0`.
- Microsoft Learn documents `IDStorageFactory` as the DirectStorage object for creating queues, status arrays, and DirectStorage files.
- Microsoft Learn documents `IDStorageQueue::EnqueueRequest`, `EnqueueStatus`, and `Submit`; status entries complete only after preceding requests complete.
- Microsoft Learn documents `DSTORAGE_QUEUE_DESC::Device` as optional; when null, request destinations must be memory. This child intentionally uses file-to-memory only and does not touch renderer/RHI/D3D12 resources.
- The existing public dispatch descriptor has only `signal_fence_after_requests` and no safe private `ID3D12Fence` handoff. This child fails closed when fence signaling is requested and leaves native D3D12 fence orchestration to a later private RHI/DirectStorage handoff child.

## Scope

### In Scope

- Add `Win32MavgPayloadDirectStorageDispatcherDesc` and `Win32MavgPayloadDirectStorageDispatcher` declarations to the existing Win32 MAVG payload IO public header without native symbols.
- Add a private DirectStorage implementation file that uses `DStorageGetFactory`, `IDStorageFactory::OpenFile`, `CreateQueue`, `CreateStatusArray`, `IDStorageQueue::EnqueueRequest`, `EnqueueStatus`, `Submit`, and `IDStorageStatusArray` polling.
- Validate only file-source to caller-owned memory-destination requests.
- Preserve `RuntimeMavgPayloadNativeIoDispatchResult` / `RuntimeMavgPayloadNativeIoStatusPollResult` evidence fields: native requests enqueued, native queue submitted, status write enqueued, `used_native_directstorage=true`, no Win32 async fallback, no background worker, no renderer/RHI handle touch.
- Extend the optional DirectStorage SDK validation test to perform a real file-to-memory queue/status dispatch when `dstorage` is installed.
- Update docs, manifest fragments, static checks, and plan registry to record the exact narrow claim.

### Out Of Scope

- `IDStorageQueue::EnqueueSignal`, `ID3D12Fence`, D3D12 resources, tiled resources, texture/buffer destinations, GPU decompression, `IDStorageQueue3::EnqueueRequests`, DirectStorage 1.4 preview features, native handle exposure, MAVG mount mutation, allocator/GPU budget enforcement, async-overlap/performance claims, Vulkan/Metal parity, mesh shaders, deformation, ray tracing, and Nanite equivalence/superiority.

## File Structure

- Modify `engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_mavg_payload_io.hpp`: add handle-free DirectStorage dispatcher declaration.
- Create `engine/runtime_host/win32/src/win32_mavg_directstorage_payload_io.cpp`: private DirectStorage SDK implementation.
- Modify `engine/runtime_host/win32/CMakeLists.txt`: add optional `MK_runtime_host_win32_directstorage`.
- Modify `CMakeLists.txt`: make `Microsoft::DirectStorage` available before the Win32 runtime-host subdirectory and link the optional test to the new adapter.
- Modify `tests/unit/runtime_host_win32_directstorage_sdk_tests.cpp`: replace pure compile/link smoke with file-to-memory native dispatch evidence while retaining header/link smoke checks.
- Modify `docs/current-capabilities.md`, `docs/superpowers/plans/README.md`, `docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md`, `engine/agent/manifest.fragments/004-modules.json`, and `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`.
- Add or update `tools/check-ai-integration-127-mavg-directstorage-native-execution.ps1` and any static-ledger registration required by the existing check system.

## Tasks

### Task 1: RED Optional DirectStorage Native Execution Test

- [x] Add a test that creates a temporary `payload.bin`, dispatches two `RuntimeMavgPayloadDirectStorageRequestRow` file-to-memory reads through `Win32MavgPayloadDirectStorageDispatcher`, polls until complete, and verifies only the requested destination ranges changed.
- [x] Add a test that sets `signal_fence_after_requests=true` and verifies the DirectStorage adapter fails before queue submission because no private fence handoff exists in this child.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset desktop-runtime`, and focused `MK_runtime_host_win32_tests` build/CTest to prove default builds remain SDK-free.
- [x] Optional `dstorage` RED is host-gated: `tools/bootstrap-deps.ps1` is command-policy approval-gated in this no-approval session, and `tools/validate-directstorage-sdk.ps1` fails at `find_package(dstorage CONFIG REQUIRED)` because `dstorage` is not installed.

### Task 2: Optional DirectStorage Adapter

- [x] Add the public PIMPL class declaration with only `std::filesystem::path`, `std::size_t`, and first-party runtime types.
- [x] Implement the private adapter with DirectStorage COM ownership, per-dispatch queue/status/file lifetime retention, relative-path validation, destination-range validation, status polling, and fail-closed diagnostics.
- [x] Ensure request debug-name strings live until the queued status completes.
- [x] Keep fence request failure deterministic and before `DStorageGetFactory`, `CreateQueue`, or `EnqueueRequest` side effects.

### Task 3: Build Wiring

- [x] Add `MK_runtime_host_win32_directstorage` only when `MK_ENABLE_DIRECTSTORAGE_SDK=ON`.
- [x] Link the optional target against `MK_runtime_host_win32` and `Microsoft::DirectStorage`.
- [x] Link `MK_runtime_host_win32_directstorage_sdk_tests` against the optional adapter and keep DirectStorage runtime DLL copy wiring.
- [x] Keep default `dev`, `desktop-runtime`, and installed SDK surfaces independent from `dstorage`.

### Task 4: Docs, Manifest, And Static Guards

- [x] Update capability docs and the MAVG master plan to claim only native file-to-memory queue/status execution.
- [x] Update plan registry and manifest fragments, then run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`.
- [x] Add/check static needles for the optional target, public-header native-symbol absence, `used_native_directstorage=true`, and the explicit native-fence non-claim.

### Task 5: Validation And Publication

- [x] Run focused C++ build/tests for default lanes touched by public headers and Win32 runtime-host code.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `tools/check-native-desktop-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-json-contracts.ps1`, `tools/check-agents.ps1`, `tools/check-format.ps1`, `tools/check-text-format.ps1`, and `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` for the C++/build/public-contract slice.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-directstorage-sdk.ps1` only when `dstorage` is installed; otherwise record the precise install blocker.
- [x] Commit validated candidate changes, run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1 -Branch codex/mavg-directstorage-native-execution-v1`, push, and open stacked draft PR #495 over `codex/mavg-d3d12-residency-action-execution-v1`.

## Validation Evidence

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset desktop-runtime` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset desktop-runtime --target MK_runtime_host_win32_tests` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset desktop-runtime --output-on-failure -R MK_runtime_host_win32_tests` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-native-desktop-contracts.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1` passed.
- `git diff --check` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-directstorage-sdk.ps1` remains host-gated here: CMake cannot find `dstorageConfig.cmake` / `dstorage-config.cmake` until the optional `directstorage-sdk` vcpkg feature is installed by an approval-capable `tools/bootstrap-deps.ps1` run.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1 -Branch codex/mavg-directstorage-native-execution-v1` passed before push.
- Published as stacked draft PR #495 over `codex/mavg-d3d12-residency-action-execution-v1`.

## Done When

- Default validation remains green without DirectStorage SDK installed.
- The optional DirectStorage SDK lane contains real native file-to-memory queue/status execution when the SDK is installed.
- Public headers and installed/default APIs expose no DirectStorage, D3D12, COM, Win32 handles, or compatibility shims.
- Docs/manifest/static checks describe the narrow implemented scope and continue to list native fence orchestration, resource destinations, GPU decompression, allocator/GPU budget enforcement, Vulkan/Metal parity, and Nanite superiority as unclaimed.
