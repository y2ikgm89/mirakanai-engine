# Frame Graph D3D12 Placed Transient Texture Lease v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Back D3D12 `IRhiDevice::acquire_transient_texture` leases with backend-private native heaps and placed resources.

**Architecture:** Keep the public transient lease contract unchanged: callers receive backend-neutral `TransientTexture` rows and release them through `release_transient`. Inside the D3D12 backend, create one private heap per transient texture lease with `ID3D12Device::CreateHeap`, then create the texture with `CreatePlacedResource` at offset `0`. Because Microsoft documents simple-model placed resources as initially inactive, record a backend-private null-before/non-null-after aliasing barrier on first command-list use to activate the placed texture. This slice proves native transient heap allocation for D3D12 only; it does not overlap multiple placed resources in one heap, insert aliasing barriers automatically into frame graph execution, expose public native handles, or claim Vulkan/Metal alias allocation.

**Tech Stack:** C++23, `MK_rhi_d3d12`, D3D12 `CreateHeap`, D3D12 `CreatePlacedResource`, PowerShell 7 validation tools.

---

## Status

**Status:** Completed.

## Official Practice Check

- Microsoft D3D12 memory-management guidance describes placed resources as resources positioned in a heap, with aliasing barriers required when overlapping placed resources reuse physical memory.
- Microsoft `ID3D12Device::CreatePlacedResource` documentation requires `GetResourceAllocationInfo` for texture resource sizes and says placed resources are created inactive in the simple model; render-target/depth-stencil aliases must be initialized by clear/discard/full subresource copy after activation.
- This slice uses `GetResourceAllocationInfo` and heap type flags for one transient texture per private heap, then activates first command-list use with a null-before/non-null-after aliasing barrier. Overlapping alias execution, overlapping alias activation scheduling, render-target/depth-stencil data inheritance beyond clear/copy use, and automatic executor insertion remain later slices.
- Source anchors: Microsoft Learn Direct3D 12 placed resources and residency guidance at `https://learn.microsoft.com/en-us/windows/win32/direct3d12/residency`, Microsoft Learn resource-barrier synchronization guidance at `https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12`, and Microsoft Learn uploading resources guidance at `https://learn.microsoft.com/en-us/windows/win32/direct3d12/uploading-resources`.

## Context

The completed Frame Graph transient texture slices added conservative alias planning, backend-neutral lease binding, shared-handle state handoff, explicit texture aliasing barrier recording, and D3D12 backend-private null-resource aliasing barrier evidence. The remaining `frame-graph-v1` foundation gap still needs native transient heap allocation before broader placed-resource alias execution and automatic executor insertion can be claimed.

## Constraints

- Do not change `IRhiDevice::acquire_transient_texture` or `TransientTexture`.
- Do not expose D3D12 heaps, placed resource pointers, native handles, or memory objects through public RHI.
- Do not overlap multiple resources into the same heap in this slice.
- Do not add automatic `execute_frame_graph_rhi_texture_schedule` aliasing-barrier insertion.
- Do not claim Vulkan memory alias allocation, Metal heaps, package streaming integration, multi-queue scheduling, or broad renderer readiness.
- Keep invalid or unsupported transient texture descriptions rejected before native allocation.

## Done When

- D3D12 device-context tests prove `create_placed_texture` creates a native heap plus placed texture without incrementing committed texture counters, and `activate_placed_texture` records only one activation barrier for repeated activation attempts.
- D3D12 `IRhiDevice::acquire_transient_texture` uses that placed texture path and records deterministic public stats evidence.
- Transient release keeps existing deferred lifetime invalidation semantics and releases placed-resource heap ownership with the resource.
- Docs, plan registry, manifest fragments, and static integration guards distinguish native D3D12 transient heap allocation from overlapping alias execution.
- Focused RHI/D3D12 tests, relevant static/agent checks, `tools/validate.ps1`, and `tools/build.ps1` pass or record a concrete environment blocker.

## Tasks

### Task 1: RED placed transient texture tests

**Files:**

- Modify: `engine/rhi/include/mirakana/rhi/rhi.hpp`
- Modify: `engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp`
- Modify: `tests/unit/d3d12_rhi_tests.cpp`

- [x] Add D3D12 device-context test expectations for `create_placed_texture`, `DeviceContextStats::placed_texture_heaps_created`, `DeviceContextStats::placed_textures_created`, and `DeviceContextStats::placed_resources_alive`.
- [x] Add D3D12 device-context activation expectations for `DeviceContext::activate_placed_texture` and `DeviceContextStats::placed_resource_activation_barriers`.
- [x] Add D3D12 `IRhiDevice` transient texture lease expectations for `RhiStats::transient_texture_heap_allocations` and `RhiStats::transient_texture_placed_allocations`.
- [x] Confirm focused `MK_d3d12_rhi_tests` fails before implementation because the new API/stat fields do not exist.

### Task 2: D3D12 native heap and placed texture implementation

**Files:**

- Modify: `engine/rhi/include/mirakana/rhi/rhi.hpp`
- Modify: `engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp`
- Modify: `engine/rhi/d3d12/src/d3d12_backend.cpp`
- Modify: `tests/unit/d3d12_rhi_tests.cpp`

- [x] Add backend-neutral transient native heap/placed allocation stats to `RhiStats`.
- [x] Add D3D12 device-context placed texture stats, `create_placed_texture`, and `activate_placed_texture`.
- [x] Keep the placed heap private and retained for the lifetime of the placed resource.
- [x] Make `D3d12RhiDevice::acquire_transient_texture` use `create_placed_texture`.
- [x] Activate placed textures on first D3D12 command-list use without claiming overlapping placed-resource alias execution.
- [x] Preserve texture descriptors, initial states, lifetime registry registration, and deferred release invalidation.
- [x] Run focused `MK_d3d12_rhi_tests` to GREEN.

### Task 3: Durable guidance and agent surface

**Files:**

- Modify: `docs/rhi.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `tools/check-ai-integration-030-runtime-rendering.ps1`

- [x] Record that D3D12 transient texture leases now use backend-private heaps, placed resources, and first-use activation barriers.
- [x] Keep overlapping alias execution, automatic aliasing-barrier insertion, Vulkan/Metal memory aliasing, public wildcard/null barriers, package streaming, and broad renderer readiness unsupported.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` after manifest changes.

### Task 4: Validation and publication

**Files:** all touched files.

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`.
- [x] Run focused `MK_rhi_tests`, `MK_d3d12_rhi_tests`, `MK_renderer_tests`, and `MK_backend_scaffold_tests`.
- [x] Run `check-format`, `check-public-api-boundaries`, `check-json-contracts`, `check-agents`, and `check-ai-integration`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Commit, push, create PR, inspect PR state/checks, and register auto-merge only after required preflight is clean.

## Validation Evidence

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_d3d12_rhi_tests'` failed before implementation because `DeviceContext::create_placed_texture`, `DeviceContextStats::placed_texture_heaps_created`, `DeviceContextStats::placed_textures_created`, `DeviceContextStats::placed_resources_alive`, `RhiStats::transient_texture_heap_allocations`, and `RhiStats::transient_texture_placed_allocations` did not exist.
- GREEN: focused `MK_d3d12_rhi_tests` build passed, and `ctest --preset dev --output-on-failure -R "MK_d3d12_rhi_tests"` passed `1/1`.
- GREEN update: focused `MK_d3d12_rhi_tests` build passed after adding first-use placed-resource activation tracking.
- REVIEW RED: after read-only rendering-auditor review, `ctest --preset dev --output-on-failure -R "MK_d3d12_rhi_tests"` failed on two targeted regressions: activation recorded on a reset-before-submit command list was incorrectly treated as active, and released transient placed resources kept `transient_texture_placed_resources_alive == 1` after deferred fence retirement.
- REVIEW GREEN: focused `MK_d3d12_rhi_tests` build passed, and `ctest --preset dev --output-on-failure -R "MK_d3d12_rhi_tests"` passed `1/1` after command-list pending activation tracking and lifetime-record-based native destruction.
- MANIFEST: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` regenerated `engine/agent/manifest.json`.
- FOCUSED: `cmake --build --preset dev --target MK_rhi_tests MK_renderer_tests MK_backend_scaffold_tests MK_d3d12_rhi_tests` passed with existing MSBuild shared-intermediate warnings; `ctest --preset dev --output-on-failure -R "MK_rhi_tests|MK_renderer_tests|MK_backend_scaffold_tests|MK_d3d12_rhi_tests"` passed `4/4`.
- STATIC: `tools/format.ps1`, `tools/check-format.ps1`, `tools/check-public-api-boundaries.ps1`, `tools/check-json-contracts.ps1`, `tools/check-agents.ps1`, and `tools/check-ai-integration.ps1` passed.
- VALIDATE: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed; diagnostic-only Metal/Apple host blockers were reported, and all `65/65` CTests passed.
- BUILD: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` passed with existing MSBuild shared-intermediate warnings.
- PUBLICATION: committed as `6d7820b`, pushed `codex/frame-graph-d3d12-placed-transient-texture-lease`, created PR #75, preflighted it as open/non-draft/mergeable with pending-only checks, and merged it into `main` as merge commit `03da3cd`.
- OFFICIAL DOCS: Context7 `/websites/learn_microsoft_en-us_windows_win32_direct3d12` was used to re-check Microsoft Learn Direct3D 12 `CreatePlacedResource`, resource heap, placed resource, and aliasing-barrier guidance before publication.
