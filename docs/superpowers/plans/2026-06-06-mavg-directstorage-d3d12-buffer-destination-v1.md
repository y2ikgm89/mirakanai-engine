# MAVG DirectStorage D3D12 Buffer Destination v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` or focused inline TDD execution to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Today's date:** 2026-06-06

**Goal:** Add a narrow optional Windows DirectStorage proof that submits MAVG payload requests into a backend-private D3D12 buffer resource destination without exposing DirectStorage, D3D12, COM, Win32, or RHI handles through public gameplay/runtime APIs.

**Architecture:** Keep default/dev/installed builds SDK-free and native-handle-free. Extend the existing optional `MK_runtime_host_win32_directstorage` target so dispatch can choose a D3D12 buffer destination proof with `DSTORAGE_REQUEST_DESTINATION_BUFFER`, a non-null `DSTORAGE_QUEUE_DESC::Device`, existing `IDStorageQueue::EnqueueRequest` / `EnqueueStatus` / `Submit`, and optional `IDStorageQueue::EnqueueSignal` fence evidence. The resource is owned privately by the optional Win32 DirectStorage dispatcher for this proof slice; full page-to-RHI resource integration remains a later child.

**Tech Stack:** C++23, Windows SDK D3D12/DXGI, Microsoft DirectStorage SDK through vcpkg `dstorage`, `Microsoft::DirectStorage`, optional CMake target-local linkage, PowerShell validation wrappers, Microsoft Learn DirectStorage and D3D12 API reference.

**Prerequisite Child:** This stacks over MAVG DirectStorage Native Fence Signal v1 (`mavg-directstorage-native-fence-signal-v1`) and keeps the same optional `MK_runtime_host_win32_directstorage` boundary.

## Official Source Audit

- Context7 does not expose a DirectStorage-specific library id. The closest Microsoft Learn corpus lookup returned unrelated storage snippets, so DirectStorage-specific facts in this child use official Microsoft Learn pages directly.
- Microsoft Learn `DSTORAGE_QUEUE_DESC` documents `ID3D12Device* Device`, and states the device is used for destination resources/GPU decompression, the destination resource must be created with the same device, and a null device is only valid with `DSTORAGE_REQUEST_DESTINATION_MEMORY`.
- Microsoft Learn `DSTORAGE_REQUEST_DESTINATION_TYPE` documents `DSTORAGE_REQUEST_DESTINATION_BUFFER` as an `ID3D12Resource` buffer destination; texture and tiled variants are separate destination types.
- Microsoft Learn `DSTORAGE_DESTINATION_BUFFER` documents the `Resource`, `Offset`, and `Size` fields used for buffer-resource destinations.
- Microsoft Learn `DSTORAGE_DESTINATION` documents that the active union member is determined by the request destination type.
- Microsoft Learn `IDStorageQueue::EnqueueRequest` documents that requests remain queued until `Submit` or queue-half-full behavior; this child keeps the existing explicit `Submit` path.
- Microsoft Learn `IDStorageQueue3::EnqueueRequests` exists for newer batched request/fence-wait synchronization. This child intentionally does not adopt `IDStorageQueue3`, tiled resources, texture regions, or GPU decompression.
- Microsoft Learn `ID3D12Device::MakeResident` and `ID3D12Device::Evict` describe explicit D3D12 residency responsibilities. This child does not broaden the existing narrow residency action proof into a full allocator or page-to-resource residency service.

## In Scope

- Add first-party request/result/status evidence for the optional D3D12 buffer destination proof, without native handle fields.
- Add a dispatch flag that selects private DirectStorage D3D12 buffer-resource destination mode.
- Preserve existing file-to-memory DirectStorage execution and fence signal behavior.
- In optional DirectStorage SDK code, create or reuse a backend-private D3D12 device, set `DSTORAGE_QUEUE_DESC::Device` non-null when buffer-resource mode is selected, create a private buffer resource, and enqueue requests with `DSTORAGE_REQUEST_DESTINATION_BUFFER`.
- Record `used_directstorage_resource_destination`, `directstorage_resource_destination_request_count`, and `directstorage_resource_destination_bytes` in dispatch/status evidence.
- Add default-lane tests that prove the first-party flags propagate through caller-owned dispatch/status boundaries without requiring the SDK.
- Add optional SDK test intent for native D3D12 buffer destination queue/status/fence proof when `dstorage` is installed.
- Update docs, plan registry, manifest fragments, composed manifest, and static guards so non-claims stay explicit.

## Out of Scope

- Public or gameplay-visible `ID3D12Resource*`, `ID3D12Device*`, `IDStorage*`, COM, Win32, or RHI native handles.
- A default DirectStorage dependency; this child keeps `dstorage` optional and records no default DirectStorage dependency.
- Binding caller-owned renderer/RHI resources as DirectStorage destinations.
- Texture, multiple-subresource, tiled-resource, or sparse/tiled residency destinations.
- GPU decompression.
- `IDStorageQueue3::EnqueueRequests` or batched request fence-wait synchronization.
- Full MAVG page-to-resource GPU residency service, allocator/GPU budget enforcement, or command-list residency-set scheduling.
- Vulkan/Metal native IO parity.
- Mesh shaders, deformation, ray tracing, benchmark superiority, async-overlap/performance claims, or Nanite compatibility/equivalence/superiority.

## Current Repository State

- `MK_runtime` already has DirectStorage-shaped request planning, native IO dispatch/status boundaries, and first-party status/fence evidence.
- `MK_runtime_host_win32_directstorage` already owns optional native DirectStorage queue/status/file-to-memory execution and private D3D12 fence signal evidence.
- `MK_runtime_rhi` / `MK_rhi` already have a narrow page-to-RHI residency action adapter and D3D12 `MakeResident` / `Evict` proof rows.
- Optional `dstorage` validation is blocked in this session: `tools/validate-directstorage-sdk.ps1` fails during `cmake --preset directstorage-sdk` because `dstorageConfig.cmake` / `dstorage-config.cmake` is not installed.

## Task 1: Runtime Contract Tests

- [x] Add a default-lane native IO test that dispatches a request plan with `use_directstorage_d3d12_buffer_destination=true` through the recording test adapter.
- [x] Assert the backend desc receives the resource-destination flag and the dispatch/status results propagate `used_directstorage_resource_destination`, request count, and byte evidence.
- [x] Assert `destination_memory` is not required in resource-destination mode at the runtime boundary.
- [x] Keep `touched_renderer_or_rhi_handles=false` and no native handle fields.

## Task 2: Runtime Contract Fields

- [x] Extend `RuntimeMavgPayloadNativeIoDispatchBackendDesc` and `RuntimeMavgPayloadNativeIoDispatchDesc` with a first-party `use_directstorage_d3d12_buffer_destination` flag.
- [x] Extend backend dispatch/status results and public dispatch/status results with first-party resource-destination evidence fields.
- [x] Wire dispatch-to-backend and backend-to-public result propagation.
- [x] Keep default values false/zero so existing memory-destination behavior remains unchanged.

## Task 3: Optional DirectStorage SDK Test Intent

- [x] Add an optional SDK test that requests D3D12 buffer destination mode, no caller destination memory, status write, and native fence signal.
- [x] Assert native DirectStorage queue submission, resource-destination evidence, status completion, and fence completion when the SDK lane can run.
- [x] Preserve unsafe source path rejection before factory creation.

## Task 4: Optional DirectStorage D3D12 Buffer Implementation

- [x] Reuse or refactor the private D3D12 device helper so DirectStorage queue creation can receive a non-null device in buffer-destination mode.
- [x] Create a backend-private D3D12 buffer resource sized to the request destination range for each pending submission.
- [x] Enqueue each request with `DSTORAGE_REQUEST_DESTINATION_BUFFER`, `Destination.Buffer.Resource`, `Destination.Buffer.Offset`, and `Destination.Buffer.Size`.
- [x] Keep file-to-memory mode using `DSTORAGE_REQUEST_DESTINATION_MEMORY` with `DSTORAGE_QUEUE_DESC::Device = nullptr`.
- [x] Preserve status array, explicit `Submit`, and optional `EnqueueSignal` ordering.
- [x] Fail closed before native calls for invalid source paths, unsupported request destination shapes, empty destination ranges, or buffer creation failures.

## Task 5: Docs, Manifest, Static Guards

- [x] Mark MAVG DirectStorage Native Fence Signal v1 publication evidence complete in the carried plan registry state.
- [x] Add this child to the plan registry and master/active plan evidence.
- [x] Update `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`, compose `engine/agent/manifest.json`, and retain `unsupportedProductionGaps = []`.
- [x] Add or update static guard needles for the new resource-destination proof and its non-claims.
- [x] Keep DirectStorage optional dependency docs unchanged unless dependency surface changes.

## Task 6: Validation and Publication

- [x] Run focused runtime tests.
- [x] Run focused optional DirectStorage configure/build/test wrapper when the SDK dependency is installed, or record the exact SDK/bootstrap blocker.
- [x] Run agent-surface/static contract checks touched by docs/manifest/static guard edits.
- [x] Run `tools/validate.ps1` for the C++/runtime/static slice before publication.
- [x] Run publication preflight, commit the validated candidate, push, and open stacked draft PR #501.

## Done When

- Default validation remains green without DirectStorage SDK installed.
- The runtime native IO boundary propagates resource-destination evidence without requiring destination memory and without exposing native handles.
- The optional DirectStorage SDK lane contains D3D12 buffer-resource destination queue/status/fence proof when the SDK is installed, or this session records the exact optional SDK bootstrap blocker.
- Docs/manifest/static checks describe the narrow implemented scope and continue to list GPU decompression, allocator/GPU budget enforcement, full page-to-resource residency service, Vulkan/Metal parity, async-overlap/performance claims, mesh shaders, deformation, ray tracing, and Nanite superiority as unclaimed.
