# MAVG Page GPU Buffer Destination Plan v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a value-only `MK_runtime_rhi` planner that maps reviewed MAVG page requests to caller-owned RHI buffer destination ranges before optional DirectStorage execution.

**Architecture:** Keep DirectStorage execution and D3D12 native objects outside `MK_runtime_rhi`. The new planner consumes a successful `RuntimeMavgPayloadDirectStorageRequestPlanResult` plus first-party `BufferHandle` destination rows, validates graph/page/mount/buffer/range consistency, and returns deterministic page-to-buffer rows that can feed the existing optional DirectStorage caller-owned RHI destination path. This is not GPU decompression, allocator enforcement, automatic RHI allocation, or a full page-to-resource residency service.

**Tech Stack:** C++23, `MK_runtime_rhi`, `MK_runtime`, `MK_rhi`, CMake/CTest, PowerShell validation wrappers, Context7 Microsoft Learn lookup, and Microsoft Learn DirectStorage Win32 API reference for `DSTORAGE_QUEUE_DESC` / `DSTORAGE_DESTINATION_BUFFER` constraints.

---

## Context

- Microsoft Learn documents that Win32 `DSTORAGE_QUEUE_DESC::Device` is used for destination resources/GPU decompression, that destination resource devices must match it, and that null device queues are limited to memory destinations.
- Microsoft Learn documents `DSTORAGE_DESTINATION_BUFFER` as an `ID3D12Resource*` plus byte `Offset` and `Size`; this plan keeps those native fields private and represents only first-party RHI buffer rows.
- PR #503 adds optional DirectStorage execution into a caller-owned D3D12 RHI `BufferHandle`, but the runtime still lacks a deterministic page-to-buffer-range planning layer.

## Files

- Create: `engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_page_gpu_buffer_destination.hpp`
- Create: `engine/runtime_rhi/src/mavg_page_gpu_buffer_destination.cpp`
- Create: `tests/unit/runtime_rhi_mavg_page_gpu_buffer_destination_tests.cpp`
- Modify: `engine/runtime_rhi/CMakeLists.txt`
- Modify: `CMakeLists.txt`
- Update: `docs/current-capabilities.md`
- Update: `docs/roadmap.md`
- Update: `docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md`
- Update: `docs/superpowers/plans/README.md`
- Update: `engine/agent/manifest.fragments/004-modules.json`
- Update: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Compose: `engine/agent/manifest.json`
- Create: `tools/check-ai-integration-131-mavg-page-gpu-buffer-destination-plan.ps1`

## Task 1: RED Planner Tests

- [x] Add `tests/unit/runtime_rhi_mavg_page_gpu_buffer_destination_tests.cpp` with:
  - a valid graph with three pages
  - a successful `RuntimeMavgPayloadDirectStorageRequestPlanResult` for pages `2, 0`
  - destination rows for page `2` and page `0` mapped into one first-party `BufferHandle`
  - assertions for output order, request indices, page indices, mount ids, buffer handle, destination offsets/sizes, total bytes, and non-claims
  - fail-closed cases for duplicate destination page rows, missing destination row for a requested page, and a request range that exceeds the supplied destination range
- [x] Add the test target to `CMakeLists.txt`.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_page_gpu_buffer_destination_tests
```

Expected: compile failure because `mirakana/runtime_rhi/mavg_buffer_destination.hpp` and planner symbols do not exist.

## Task 2: GREEN Planner API And Implementation

- [x] Add `mavg_page_gpu_buffer_destination.hpp` with:
  - `RuntimeMavgPageBufferDestinationDiagnosticCode`
  - `RuntimeMavgPageBufferDestinationDiagnostic`
  - `RuntimeMavgPageBufferDestinationRow`
  - `RuntimeMavgPageBufferDestinationDesc`
  - `RuntimeMavgPageBufferDestinationPlanRow`
  - `RuntimeMavgPageBufferDestinationPlanResult`
  - `plan_runtime_mavg_page_buffer_destinations`
- [x] Implement `mavg_page_gpu_buffer_destination.cpp` to validate:
  - non-zero `graph_asset`
  - non-null successful request plan
  - request plan diagnostics empty
  - graph present, asset-matching, and valid
  - every destination row matches the graph asset, references a graph page, has non-zero mount id, buffer handle, and size
  - destination rows are unique by page index and mount id
  - every planned request has a matching destination row
  - request destination range fits the matching destination row
- [x] Keep result non-claims explicit:
  - `invoked_file_io=false`
  - `used_native_directstorage=false`
  - `submitted_native_queue=false`
  - `used_directstorage_resource_destination=false`
  - `used_gpu_decompression=false`
  - `allocated_rhi_resources=false`
  - `enforced_allocator_budget=false`
  - `mutated_mount_set=false`
  - `exposed_native_handles=false`
- [x] Add the source to `engine/runtime_rhi/CMakeLists.txt`.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_page_gpu_buffer_destination_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_rhi_mavg_page_gpu_buffer_destination_tests
```

Expected: PASS.

## Task 3: Docs, Manifest, And Static Guard

- [x] Update roadmap/current-capabilities/MAVG master plan/plan registry with exact evidence and non-claims.
- [x] Update manifest fragments and compose:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
```

- [x] Add `tools/check-ai-integration-131-mavg-page-gpu-buffer-destination-plan.ps1` to assert the new API, test target, docs, manifest evidence, and non-claims.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
```

## Task 4: Validation And Publication

- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1
```

- [ ] Commit, push, and open a stacked draft PR against `codex/mavg-directstorage-rhi-resource-destination-v1`.

## Done When

- `MK_runtime_rhi` can produce deterministic first-party page-to-buffer destination rows for a successful MAVG DirectStorage request plan.
- Invalid page/mount/buffer/range rows fail before file IO, native DirectStorage, RHI allocation, mount mutation, or native handle exposure.
- Docs, manifest fragments, composed manifest, static guards, focused tests, and full validation match the exact narrow evidence.
- GPU decompression, full page-to-resource GPU residency service, automatic RHI allocation, allocator/GPU budget enforcement, Vulkan/Metal native IO parity, mesh shaders, Nanite compatibility/equivalence/superiority, async-overlap/performance, and broad optimization remain unclaimed.

## Validation Evidence

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_page_gpu_buffer_destination_tests` initially failed before configure because the new linked worktree had no `out/build/dev` tree.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_page_gpu_buffer_destination_tests` then failed as RED because `mirakana/runtime_rhi/mavg_page_gpu_buffer_destination.hpp` did not exist.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_page_gpu_buffer_destination_tests` passed after adding `mavg_page_gpu_buffer_destination.hpp` / `.cpp`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_rhi_mavg_page_gpu_buffer_destination_tests` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_page_gpu_buffer_destination_tests MK_runtime_rhi_mavg_residency_tests MK_runtime_mavg_payload_pages_tests; pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_rhi_mavg_page_gpu_buffer_destination_tests|MK_runtime_rhi_mavg_residency_tests|MK_runtime_mavg_payload_pages_tests"` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` passed and wrote `engine/agent/manifest.json`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` initially failed because the new Chapter 131 guard expected `mavg_page_gpu_buffer_destination.hpp` in `engine/runtime_rhi/CMakeLists.txt`; the guard was corrected to match the existing source-only runtime RHI CMake style.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` then failed because the MAVG master plan did not yet name `mavg_page_gpu_buffer_destination.hpp`; the master plan was updated with the exact API evidence.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` initially failed on `engine/runtime_rhi/src/mavg_page_gpu_buffer_destination.cpp`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` passed and normalized the C++ formatting.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed. Static checks all passed, `production-readiness-audit` reported `unsupported_gaps=0`, diagnostic-only Metal/Apple/mobile host gates remained host-gated as expected on this Windows host, build passed, clang-tidy smoke passed, and CTest passed `111/111` including `MK_runtime_rhi_mavg_page_gpu_buffer_destination_tests`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1` passed with branch `codex/mavg-page-gpu-buffer-destination-plan-v1`, GitHub auth/network ready, and remote head missing as expected before first push.
