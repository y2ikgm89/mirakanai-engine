# MAVG DirectStorage Native Fence Signal v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a narrow optional Windows DirectStorage native fence signal proof for MAVG file-to-memory payload IO without exposing DirectStorage, D3D12, COM, Win32, or RHI handles through public gameplay/runtime APIs.

**Architecture:** Keep `MK_runtime` and default builds SDK-free and handle-free. Extend only the optional `MK_runtime_host_win32_directstorage` target so `signal_fence_after_requests=true` creates a backend-private `ID3D12Fence`, enqueues `IDStorageQueue::EnqueueSignal` after file-to-memory requests and before status polling, and reports first-party numeric fence evidence through the existing `IRuntimeMavgPayloadNativeIoDispatcher` result surface.

**Tech Stack:** C++23, Windows SDK D3D12/DXGI, Microsoft DirectStorage SDK through vcpkg `dstorage`, `Microsoft::DirectStorage`, CMake target-local optional linking, PowerShell validation wrappers, Microsoft Learn DirectStorage API reference, Context7 Microsoft Learn lookup.

**Prerequisite Child:** This is a focused follow-up over MAVG DirectStorage Native Execution v1 (`mavg-directstorage-native-execution-v1`) and keeps the same optional `MK_runtime_host_win32_directstorage` boundary.

---

## Official Source Audit

- Context7 does not expose a DirectStorage-specific library id. The closest high-reputation result is `/websites/learn_microsoft_en-us`; querying it for DirectStorage queue/fence APIs returned unrelated Azure Storage snippets, so DirectStorage-specific facts in this child use official Microsoft Learn pages directly.
- Microsoft Learn `IDStorageQueue::EnqueueSignal` documents `void EnqueueSignal(ID3D12Fence* fence, UINT64 value)`, with the fence write occurring after all requests before the fence entry complete.
- Microsoft Learn `IDStorageQueue::EnqueueStatus` documents status writes after preceding requests complete, and `IDStorageQueue::Submit` submits enqueued work for execution.
- Microsoft Learn `IDStorageQueue3::EnqueueRequests` and `DSTORAGE_ENQUEUE_REQUEST_FLAGS` describe newer request-array fence-wait synchronization. This child intentionally does not adopt `IDStorageQueue3`, batched request synchronization, resource destinations, or GPU decompression.

## Scope

### In Scope

- Use a private `ID3D12Fence` only inside `engine/runtime_host/win32/src/win32_mavg_directstorage_payload_io.cpp`.
- Create the private fence with a D3D12 device, using default adapter first and DXGI WARP fallback when needed.
- Enqueue `IDStorageQueue::EnqueueSignal` when `signal_fence_after_requests=true`.
- Add first-party `native_fence_signal_value` and `native_fence_completed_value` result fields to the existing native IO dispatch/status result contracts.
- Preserve public header native-symbol isolation and keep optional DirectStorage/D3D12 linkage target-local to `MK_runtime_host_win32_directstorage`.
- Update optional SDK tests to cover file-to-memory queue/status/fence signal execution when `dstorage` is installed.
- Update docs, plan registry, manifest fragments, and static guards so the claim is exact and narrow.

### Out Of Scope

- Public native handles, renderer/RHI fence handoff, D3D12 resource destinations, tiled resources, GPU decompression, `IDStorageQueue3::EnqueueRequests`, allocator/GPU budget enforcement, residency/mount mutation, Vulkan/Metal parity, async-overlap/performance claims, mesh shaders, deformation, ray tracing, benchmark superiority, or Nanite compatibility/equivalence/superiority.

## Tasks

### Task 1: RED/Green Evidence

- [x] Update optional DirectStorage SDK test intent from `signal_fence_after_requests` fail-closed to native fence signal execution.
- [x] Add default-lane runtime native IO adapter assertions for `native_fence_signal_value` / `native_fence_completed_value` propagation.
- [x] Run focused default builds/tests after implementation.
- [x] Run `tools/validate-directstorage-sdk.ps1` when `dstorage` is installed, or record the exact bootstrap/configure blocker.

### Task 2: Optional DirectStorage Fence Implementation

- [x] Retain a backend-private `ID3D12Fence` per pending DirectStorage submission.
- [x] Create a backend-private D3D12 fence device using default adapter then DXGI WARP fallback.
- [x] Enqueue `IDStorageQueue::EnqueueSignal` after file-to-memory requests and before `EnqueueStatus`.
- [x] Report `signaled_native_fence`, `native_fence_signal_value`, and `native_fence_completed_value` on dispatch and status polling.
- [x] Keep `touched_renderer_or_rhi_handles=false`; this is a private runtime-host adapter fence, not renderer/RHI interop.

### Task 3: Build Wiring

- [x] Link optional `MK_runtime_host_win32_directstorage` privately with `d3d12` and `dxgi`.
- [x] Keep default `dev`, `desktop-runtime`, and installed SDK surfaces independent from `dstorage`, `d3d12`, and `dxgi` optional adapter linkage.

### Task 4: Docs, Manifest, And Static Guards

- [x] Update current capabilities, roadmap, MAVG spec/master plan, production master plan, and plan registry.
- [x] Update `engine/agent/manifest.fragments/004-modules.json`, `009-validationRecipes.json`, and `010-aiOperableProductionLoop.json`, then compose `engine/agent/manifest.json`.
- [x] Update `tools/check-ai-integration-127-mavg-directstorage-native-execution.ps1` static needles for the new fence signal child.

### Task 5: Validation And Publication

- [x] Run focused C++ build/tests.
- [x] Run agent-surface/static validation after docs and manifest edits.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [ ] Run publication preflight, commit the validated candidate, push, and open a stacked draft PR.

## Validation Evidence

- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev`
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_tests`
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_tests`
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset desktop-runtime`
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset desktop-runtime --target MK_runtime_host_win32_tests`
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset desktop-runtime --output-on-failure -R MK_runtime_host_win32_tests`
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-native-desktop-contracts.ps1`
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1`
- PASS: `git diff --check`
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`
- BLOCKED optional SDK lane: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-directstorage-sdk.ps1` cannot configure because `dstorageConfig.cmake` / `dstorage-config.cmake` is not installed, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1` is blocked in this session by approval policy (`AskForApproval is set to Never`).

## Done When

- Default validation remains green without DirectStorage SDK installed.
- The optional DirectStorage SDK lane contains native file-to-memory queue/status/fence signal execution when the SDK is installed, or this session records the exact optional SDK bootstrap blocker.
- Public headers and installed/default APIs expose no DirectStorage, D3D12, COM, Win32 handles, or compatibility shims.
- Docs/manifest/static checks describe the narrow implemented scope and continue to list D3D12 resource destinations, GPU decompression, allocator/GPU budget enforcement, Vulkan/Metal parity, async-overlap/performance claims, and Nanite superiority as unclaimed.
