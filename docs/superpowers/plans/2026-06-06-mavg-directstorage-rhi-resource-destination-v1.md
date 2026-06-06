# 2026-06-06 MAVG DirectStorage RHI Resource Destination v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a narrow optional DirectStorage path that writes MAVG payload requests into a caller-owned D3D12 RHI `BufferHandle` destination without exposing `ID3D12Device`, `ID3D12Resource`, DirectStorage objects, or public native handles.

**Architecture:** Keep default builds and `MK_runtime` DirectStorage/RHI-free. Extend the optional Win32 DirectStorage dispatcher descriptor with first-party `mirakana::rhi::IRhiDevice` plus `BufferHandle` destination fields, resolve the D3D12 resource only through private `MK_rhi_d3d12` interop, and feed `DSTORAGE_REQUEST_DESTINATION_BUFFER` with a queue `Device` that matches the resolved resource device. Preserve the existing private buffer destination proof and distinguish caller-owned RHI destination evidence with new first-party counters.

**Tech Stack:** C++23, `MK_runtime`, `MK_runtime_host_win32_directstorage`, `MK_rhi`, private `MK_rhi_d3d12` interop, Microsoft DirectStorage SDK `dstorage.h`, Microsoft Learn DirectStorage API reference, CMake/CTest, PowerShell validation wrappers, Context7 Microsoft Learn lookup, and Microsoft WARP-backed D3D12 tests.

---

## Official Source Audit

- Context7 query for Microsoft DirectStorage returned only the broad Microsoft Learn corpus, not a DirectStorage-specific library id. Use Microsoft Learn API reference as the exact source of truth for DirectStorage symbols.
- Microsoft Learn `DSTORAGE_QUEUE_DESC` states `Device` is the optional device used for destination resources/GPU decompression, that destination resources must match this device, and that null device restricts destinations to `DSTORAGE_REQUEST_DESTINATION_MEMORY`.
- Microsoft Learn `DSTORAGE_DESTINATION_BUFFER` defines a buffer destination as `ID3D12Resource* Resource`, byte `Offset`, and byte `Size`.
- Microsoft Learn `DSTORAGE_DESTINATION` selects the active destination union member from `request.Options.DestinationType`.
- Microsoft Learn `IDStorageQueue::EnqueueRequest` enqueues requests until `Submit` or automatic submission.

## Current Project Audit

- `docs/superpowers/plans/2026-06-06-mavg-directstorage-d3d12-buffer-destination-v1.md` and draft PR #501 completed a private D3D12 buffer destination proof behind `MK_runtime_host_win32_directstorage`.
- `engine/runtime/include/mirakana/runtime/mavg_payload_pages.hpp` already carries handle-free native IO dispatch/status counters for DirectStorage resource destinations.
- `engine/runtime_host/win32/src/win32_mavg_directstorage_payload_io.cpp` currently creates a private `ID3D12Resource` buffer when `use_directstorage_d3d12_buffer_destination=true`.
- `engine/rhi/d3d12/src/d3d12_backend.cpp` keeps `D3d12RhiDevice` and `NativeResourceHandle` private while resolving `BufferHandle`/`TextureHandle` internally for residency actions.
- `engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_residency.hpp` proves page-to-RHI residency action rows but still reports `used_directstorage_resource_destination=false`.

## Scope

- Add caller-owned D3D12 RHI buffer destination proof for optional DirectStorage SDK builds.
- Keep `MK_runtime` default public contract handle-free.
- Keep private buffer destination mode working.
- Add exact evidence fields for caller-owned RHI destination usage.
- Validate RHI destination range/usage/backend before DirectStorage factory/queue work.

## Non-Goals

- GPU decompression.
- Texture/tile/multiple-subresource DirectStorage destinations.
- Vulkan/Metal native IO parity.
- Full MAVG page-to-resource GPU residency service.
- Allocator/GPU budget enforcement.
- Sustained async-overlap/performance or benchmark superiority.
- Mesh shaders, deformation, ray tracing, or Nanite compatibility/equivalence/superiority.

## Files

- Modify: `engine/runtime/include/mirakana/runtime/mavg_payload_pages.hpp`
- Modify: `engine/runtime/src/mavg_payload_pages.cpp`
- Modify: `engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_mavg_payload_io.hpp`
- Modify: `engine/runtime_host/win32/src/win32_mavg_directstorage_payload_io.cpp`
- Add: `engine/rhi/d3d12/src/d3d12_directstorage_private.hpp`
- Modify: `engine/rhi/d3d12/src/d3d12_backend.cpp`
- Modify: `engine/runtime_host/win32/CMakeLists.txt`
- Modify: `CMakeLists.txt`
- Modify: `tests/unit/runtime_host_win32_directstorage_sdk_tests.cpp`
- Modify: `tests/unit/runtime_mavg_payload_pages_tests.cpp`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify or add: `tools/check-ai-integration-130-mavg-directstorage-rhi-resource-destination.ps1`

## Task 1: RED Runtime Evidence Fields

- [ ] Add failing test coverage in `tests/unit/runtime_mavg_payload_pages_tests.cpp` proving a test adapter can return:
  - `used_directstorage_resource_destination=true`
  - `used_directstorage_caller_owned_rhi_resource_destination=true`
  - `directstorage_resource_destination_request_count` equal to planned request count
  - `directstorage_resource_destination_bytes` equal to planned destination bytes
  - `touched_renderer_or_rhi_handles=true`
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_payload_pages_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_mavg_payload_pages_tests"
```

Expected: compile/test failure because `used_directstorage_caller_owned_rhi_resource_destination` does not exist.

## Task 2: GREEN Runtime Evidence Fields

- [ ] Add `bool used_directstorage_caller_owned_rhi_resource_destination{false};` to:
  - `RuntimeMavgPayloadNativeIoDispatchBackendResult`
  - `RuntimeMavgPayloadNativeIoStatusBackendResult`
  - `RuntimeMavgPayloadNativeIoDispatchResult`
  - `RuntimeMavgPayloadNativeIoStatusPollResult`
- [ ] Propagate the field through `dispatch_runtime_mavg_payload_native_io_requests` and `poll_runtime_mavg_payload_native_io_status`.
- [ ] Keep existing private buffer destination tests green with the new field false.
- [ ] Run the Task 1 commands and expect PASS.

## Task 3: RED D3D12 Private Resolver

- [ ] Add a failing D3D12 unit test in `tests/unit/d3d12_rhi_tests.cpp` that:
  - creates a D3D12 RHI device through `mirakana::rhi::d3d12::create_rhi_device`
  - creates a caller-owned buffer with `BufferUsage::storage | BufferUsage::copy_source | BufferUsage::copy_destination`
  - calls private test-only/compiled helper declared in `engine/rhi/d3d12/src/d3d12_directstorage_private.hpp`
  - verifies the result reports D3D12 backend, valid device/resource, `size_bytes`, `copy_destination` compatibility, `exposed_native_handles=false`, and no DirectStorage execution.
- [ ] Add a failing test for unsupported backend / invalid buffer returning fail-closed diagnostics without native pointer exposure.
- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_d3d12_rhi_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_d3d12_rhi_tests"
```

Expected: compile failure because `d3d12_directstorage_private.hpp` and resolver symbols do not exist.

## Task 4: GREEN D3D12 Private Resolver

- [ ] Add `engine/rhi/d3d12/src/d3d12_directstorage_private.hpp` with a private `mirakana::rhi::d3d12::native` resolver result containing `ComPtr<ID3D12Device>`, `ComPtr<ID3D12Resource>`, `BufferDesc`, `size_bytes`, diagnostic flags, and `exposed_native_handles=false`.
- [ ] Implement `resolve_directstorage_buffer_destination(IRhiDevice&, BufferHandle) noexcept` in `d3d12_backend.cpp` by `dynamic_cast` to private `D3d12RhiDevice`, validating:
  - backend is D3D12
  - buffer is owned and resident
  - buffer has `BufferUsage::copy_destination`
  - native D3D12 resource is a buffer
  - resource heap is suitable for GPU resource destination, not pure upload/readback helper
  - `ID3D12Resource::GetDevice` succeeds
- [ ] Return `ComPtr` references so pending DirectStorage submissions keep the caller-owned resource alive until completion.
- [ ] Run the Task 3 commands and expect PASS.

## Task 5: RED DirectStorage RHI Dispatcher

- [ ] Add failing optional SDK test in `tests/unit/runtime_host_win32_directstorage_sdk_tests.cpp` that:
  - creates a D3D12 RHI device
  - creates a caller-owned RHI buffer destination sized larger than the request plan
  - dispatches the existing request plan through `Win32MavgPayloadDirectStorageDispatcher` configured with `directstorage_rhi_device` and `directstorage_rhi_destination_buffer`
  - sets `use_directstorage_d3d12_buffer_destination=true`
  - verifies dispatch/status complete with:
    - `used_directstorage_resource_destination=true`
    - `used_directstorage_caller_owned_rhi_resource_destination=true`
    - `directstorage_resource_destination_request_count=2`
    - `directstorage_resource_destination_bytes=8`
    - `touched_renderer_or_rhi_handles=true`
    - no public native handle fields
  - copies the caller-owned default buffer to a readback buffer through public RHI commands and verifies bytes landed at offsets 3 and 10.
- [ ] Add failing tests for missing RHI device, unsupported backend, invalid destination buffer, and too-small destination range before DirectStorage queue creation.
- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-directstorage-sdk.ps1
```

Expected: compile failure if DirectStorage SDK is installed; otherwise the same existing SDK configure blocker is recorded.

## Task 6: GREEN DirectStorage RHI Dispatcher

- [ ] Extend existing `Win32MavgPayloadDirectStorageDispatcherDesc` with `directstorage_rhi_device` and `directstorage_rhi_destination_buffer` under `MK_RUNTIME_HOST_WIN32_ENABLE_DIRECTSTORAGE_SDK`.
- [ ] Implement the optional external D3D12 resource destination path in `win32_mavg_directstorage_payload_io.cpp`, sharing the existing DirectStorage submission logic while preserving file-to-memory and private resource destination modes.
- [ ] Use the resolved resource's `ID3D12Device` for `DSTORAGE_QUEUE_DESC::Device`.
- [ ] Store `ComPtr<ID3D12Device>` and `ComPtr<ID3D12Resource>` in `DirectStoragePendingSubmission`.
- [ ] Create completion fences on the same resolved device when `signal_fence_after_requests=true`.
- [ ] Reject unsupported/missing/too-small RHI destinations before `DStorageGetFactory`.
- [ ] Set caller-owned RHI evidence fields only for this path; leave private buffer destination fields compatible.
- [ ] Keep `used_gpu_decompression=false` and no performance/benchmark fields.
- [ ] Run the Task 5 command. If SDK is not installed, run default focused compile/tests that do not require the SDK and record the SDK blocker.

## Task 7: CMake Wiring

- [ ] Link `MK_runtime_host_win32_directstorage` or a new optional target to public `MK_rhi` and private `MK_rhi_d3d12` only when `MK_ENABLE_DIRECTSTORAGE_SDK` and `TARGET MK_rhi_d3d12` are true.
- [ ] Add the new source/header to the appropriate target.
- [ ] Update `MK_runtime_host_win32_directstorage_sdk_tests` links for RHI/D3D12.
- [ ] Preserve default SDK-free builds.
- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_payload_pages_tests MK_d3d12_rhi_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_mavg_payload_pages_tests|MK_d3d12_rhi_tests"
```

## Task 8: Docs, Manifest, And Static Guard Sync

- [x] Update `docs/current-capabilities.md`, `docs/roadmap.md`, plan registry, and MAVG master plan with exact completed evidence and remaining non-claims.
- [x] Update `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`, then run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
```

- [x] Add/update static guard chapter `tools/check-ai-integration-130-mavg-directstorage-rhi-resource-destination.ps1` to assert:
  - new public header/class names
  - private D3D12 resolver symbol
  - caller-owned RHI evidence field
  - docs/manifest non-claims for GPU decompression, allocator/GPU budget enforcement, Vulkan/Metal parity, async-overlap/performance, and Nanite claims
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
```

## Task 9: Slice Validation And Publication

- [x] Run focused checks:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_payload_pages_tests MK_d3d12_rhi_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_mavg_payload_pages_tests|MK_d3d12_rhi_tests"
```

- [x] Run full validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

- [x] Run optional SDK validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-directstorage-sdk.ps1
```

Expected: PASS on a host with installed `dstorage`; if blocked by missing `dstorageConfig.cmake`, record the exact blocker in this plan and PR body.

- [x] Run publication preflight:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1
```

- [x] Commit, push, and open stacked draft PR #503 against `codex/mavg-directstorage-d3d12-buffer-destination-v1`.

## Done When

- Optional DirectStorage SDK path can write file request rows into a caller-owned D3D12 RHI `BufferHandle` as `DSTORAGE_REQUEST_DESTINATION_BUFFER`.
- Public APIs expose only first-party `IRhiDevice`/`BufferHandle` and no `ID3D12Device`, `ID3D12Resource`, COM, Win32, or DirectStorage handles.
- Dispatch/status results distinguish private D3D12 buffer destination from caller-owned RHI destination.
- Existing private buffer destination and file-to-memory tests remain green.
- Default builds remain DirectStorage SDK-free.
- Docs, manifest fragments, composed manifest, and static guards match the evidence.
- Full validation passes or concrete host/toolchain blockers are recorded.

## Validation Evidence

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_payload_pages_tests`
  initially failed because `used_directstorage_caller_owned_rhi_resource_destination` did not exist.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_d3d12_rhi_tests`
  initially failed because `d3d12_directstorage_private.hpp` did not exist.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_payload_pages_tests MK_d3d12_rhi_tests; pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_mavg_payload_pages_tests|MK_d3d12_rhi_tests"` passed after the runtime evidence fields and D3D12 private resolver were implemented.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after adding `tools/check-ai-integration-130-mavg-directstorage-rhi-resource-destination.ps1`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed after composing `engine/agent/manifest.json`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed with DirectStorage/D3D12/COM handles kept out of public headers.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed, including all 110 CTest tests.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-directstorage-sdk.ps1` is host/dependency-blocked on this machine because CMake cannot find `dstorage.cps`, `dstorageConfig.cmake`, or `dstorage-config.cmake`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1` passed.
- Published as stacked draft PR #503 against `codex/mavg-directstorage-d3d12-buffer-destination-v1`.
