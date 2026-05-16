# 2026-05-17 Frame Graph Backend-Neutral Distinct Alias-Group Lease Binding v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Bind Frame Graph transient texture alias groups to backend-neutral RHI alias-group leases that return distinct `TextureHandle` rows per resource.

**Architecture:** Add a first-party RHI value contract for one transient texture alias-group lease with N backend-neutral texture handles. `MK_renderer` keeps the existing alias plan boundary, but its lease binding calls the new RHI alias-group acquisition once per alias group and maps each resource name to a distinct texture handle so explicit aliasing barriers can be recorded. D3D12 uses the existing backend-private same-offset placed texture alias group path for multi-resource groups; NullRHI and Vulkan use deterministic distinct texture handles without claiming native memory alias allocation.

**Tech Stack:** C++23, `MK_rhi`, `MK_rhi_d3d12`, `MK_rhi_vulkan`, `MK_renderer`, D3D12 `CreatePlacedResource` / `D3D12_RESOURCE_ALIASING_BARRIER`, PowerShell 7 validation tools.

---

## Status

**Status:** Completed for implementation and validation. Publication is handled by the task branch PR workflow.

## Official Practice Check

- Context7 `/websites/learn_microsoft_en-us_windows_win32_direct3d12` was used on 2026-05-17 to re-check Microsoft Learn Direct3D 12 placed-resource aliasing guidance.
- Microsoft Learn describes `D3D12_RESOURCE_ALIASING_BARRIER` as the transition between usages of two different resources mapped into the same heap; this slice keeps those details backend-private.
- Microsoft Learn `ID3D12Device::CreatePlacedResource` guidance supports creating resources inside an explicit heap, and resource-barrier guidance requires aliasing barriers when overlapping mappings switch usage.
- This slice does not claim data inheritance/content preservation. Later executor or renderer users must still clear, discard, copy, or fully overwrite according to the documented D3D12 memory aliasing constraints before relying on contents.
- Source anchors: `https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12`, `https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_resource_aliasing_barrier`, and `https://learn.microsoft.com/en-us/windows/win32/direct3d12/memory-aliasing-and-data-inheritance`.

## Context

The completed Frame Graph lease-binding slice binds one transient texture lease per alias group, but it assigns the same `TextureHandle` to every resource in that group. The completed explicit aliasing-barrier helper intentionally rejects same-handle barriers, and the completed D3D12 backend-private alias group slice proves distinct same-offset placed texture handles can record non-null D3D12 aliasing barriers. The next prerequisite for automatic aliasing-barrier insertion is a backend-neutral RHI and Frame Graph binding surface that can provide distinct handles while retaining one alias-group lease lifetime.

## Constraints

- Do not expose D3D12 heaps, resources, descriptors, offsets, memory objects, queues, fences, or native handles through public RHI/gameplay APIs.
- Do not add automatic aliasing-barrier insertion to `execute_frame_graph_rhi_texture_schedule`.
- Do not add public wildcard/null aliasing barriers.
- Do not claim Vulkan/Metal native memory alias allocation, data inheritance/content preservation, package streaming integration, multi-queue graph scheduling, production render graph scheduling, or broad renderer readiness.
- Preserve clean breaking semantics: update tests and docs to the new distinct-handle contract instead of preserving the old shared-handle alias-group binding behavior.

## Done When

- `IRhiDevice` exposes a backend-neutral `acquire_transient_texture_alias_group(TextureDesc, std::size_t)` value contract with one releaseable lease and distinct texture handles.
- NullRHI and Vulkan return deterministic distinct texture handles under one lease without claiming native alias memory.
- D3D12 RHI uses backend-private placed texture alias groups for multi-resource alias-group leases and keeps one-texture groups on the existing placed transient texture path.
- `acquire_frame_graph_transient_texture_lease_bindings` returns one `FrameGraphTransientTextureLease` per alias group and distinct `FrameGraphTextureBinding` handles per resource in that group.
- Explicit `record_frame_graph_texture_aliasing_barriers` succeeds for resources returned from one alias group because their texture handles are distinct.
- Docs, plan registry, manifest fragments, skills/subagents, and static integration guards distinguish backend-neutral distinct alias-group lease binding from automatic insertion, Vulkan/Metal memory aliasing, content preservation, package streaming, and broad renderer readiness.
- Focused RHI/renderer/D3D12 tests, relevant static/agent checks, `tools/validate.ps1`, and `tools/build.ps1` pass or record a concrete environment blocker.

## Tasks

### Task 1: RED RHI and Frame Graph contract tests

**Files:**

- Modify: `tests/unit/rhi_tests.cpp`
- Modify: `tests/unit/renderer_rhi_tests.cpp`
- Modify: `tests/unit/d3d12_rhi_tests.cpp`
- Modify: `tests/unit/runtime_host_tests.cpp`

- [x] Add NullRHI test coverage for `acquire_transient_texture_alias_group(desc, 2)` returning one non-zero lease, two distinct texture handles, `textures_created == 2`, `transient_resources_acquired == 1`, and release invalidating both texture handles.
- [x] Add D3D12 RHI test coverage for `acquire_transient_texture_alias_group(desc, 2)` returning two distinct handles, recording an explicit texture aliasing barrier between them, and retiring both placed resources after one lease release and fence completion.
- [x] Update `ThrowingSubmitRhiDevice`, `ThrowingTransientTextureAcquireRhiDevice`, and runtime-host test fakes for the new `IRhiDevice` method.
- [x] Change the existing Frame Graph lease-binding success test to require distinct handles for resources in the same alias group while preserving one lease per alias group.
- [x] Add Frame Graph explicit aliasing-barrier proof using the two same-group resource names returned by lease binding.
- [x] Update partial-failure coverage so a failing second alias-group acquisition releases the first acquired alias-group lease.
- [x] Confirm focused `MK_rhi_tests`, `MK_renderer_tests`, `MK_d3d12_rhi_tests`, and `MK_runtime_host_tests` fail before implementation because the new API or distinct-handle contract does not exist.

### Task 2: RHI alias-group lease implementation

**Files:**

- Modify: `engine/rhi/include/mirakana/rhi/rhi.hpp`
- Modify: `engine/rhi/src/null_rhi.cpp`
- Modify: `engine/rhi/d3d12/src/d3d12_backend.cpp`
- Modify: `engine/rhi/vulkan/src/vulkan_backend.cpp`

- [x] Add `rhi::TransientTextureAliasGroup` with `TransientResourceHandle lease` and `std::vector<TextureHandle> textures`.
- [x] Add `IRhiDevice::acquire_transient_texture_alias_group(const TextureDesc& desc, std::size_t texture_count)`.
- [x] Implement NullRHI alias-group acquisition by validating `texture_count > 0`, creating one texture per requested handle, storing all handles under one transient lease record, and releasing every handle from `release_transient`.
- [x] Implement Vulkan fallback acquisition with one backend-neutral lease and distinct regular texture handles, keeping native memory alias allocation unsupported.
- [x] Implement D3D12 acquisition so `texture_count == 1` wraps the existing `acquire_transient_texture`, while `texture_count > 1` uses `DeviceContext::create_placed_texture_alias_group`, registers each texture lifetime row, stores all handles under one transient lease record, and releases all of them from `release_transient`.
- [x] Preserve existing one-texture transient stats and update multi-texture group stats honestly: one transient resource acquired/active/released per lease, one heap allocation for a D3D12 alias group, and one placed allocation/alive row per texture.
- [x] Run focused RHI/D3D12 tests to GREEN.

### Task 3: Frame Graph distinct binding implementation

**Files:**

- Modify: `engine/renderer/include/mirakana/renderer/frame_graph_rhi.hpp`
- Modify: `engine/renderer/src/frame_graph_rhi.cpp`
- Modify: `tests/unit/renderer_rhi_tests.cpp`

- [x] Change `FrameGraphTransientTextureLease` to retain the alias-group `rhi::TransientTextureAliasGroup`.
- [x] Make `acquire_frame_graph_transient_texture_lease_bindings` call `device.acquire_transient_texture_alias_group(group.desc, group.resources.size())` once per alias group.
- [x] Emit `FrameGraphTextureBinding` rows in group resource order, mapping each resource to the matching distinct handle from the returned alias group and `ResourceState::undefined`.
- [x] Reject malformed empty resource names before acquisition, preserve plan diagnostics without acquisition, and release already acquired alias-group leases after later acquisition failure.
- [x] If a backend returns the wrong number of texture handles, release all acquired leases and report an `invalid_resource` diagnostic instead of emitting partial bindings.
- [x] Run focused `MK_renderer_tests` to GREEN.

### Task 4: Durable guidance and agent surface

**Files:**

- Modify: `docs/rhi.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `engine/agent/manifest.json` via compose only
- Modify: `.agents/skills/rendering-change/references/full-guidance.md`
- Modify: `.claude/skills/gameengine-rendering/references/full-guidance.md`
- Modify: `.codex/agents/rendering-auditor.toml`
- Modify: `.claude/agents/rendering-auditor.md`
- Modify: `tools/check-ai-integration-030-runtime-rendering.ps1`

- [x] Record the new backend-neutral distinct alias-group lease binding contract.
- [x] Record that D3D12 multi-texture alias-group leases use backend-private same-offset placed texture handles and explicit aliasing barriers can now target distinct Frame Graph bindings.
- [x] Keep automatic executor insertion, public wildcard/null barriers, Vulkan/Metal memory aliasing, content preservation, package streaming, multi-queue scheduling, and broad renderer readiness unsupported.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`.

### Task 5: Validation and publication

**Files:** all touched files.

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`.
- [x] Run focused build: `cmake --build --preset dev --target MK_rhi_tests MK_renderer_tests MK_d3d12_rhi_tests MK_runtime_host_tests MK_backend_scaffold_tests`.
- [x] Run focused tests: `ctest --preset dev --output-on-failure -R "MK_rhi_tests|MK_renderer_tests|MK_d3d12_rhi_tests|MK_runtime_host_tests|MK_backend_scaffold_tests"`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [ ] Commit, push, create PR, inspect PR state/checks, and register auto-merge only after required preflight is clean.

## Validation Evidence

- OFFICIAL DOCS: Context7 `/websites/learn_microsoft_en-us_windows_win32_direct3d12` was used to re-check Microsoft Learn Direct3D 12 `CreatePlacedResource`, `D3D12_RESOURCE_ALIASING_BARRIER`, and resource-barrier guidance before implementation.
- RED: `cmake --build --preset dev --target MK_rhi_tests MK_renderer_tests MK_d3d12_rhi_tests MK_runtime_host_tests` failed before implementation because `IRhiDevice::acquire_transient_texture_alias_group` did not exist.
- GREEN focused build: `cmake --build --preset dev --target MK_rhi_tests MK_renderer_tests MK_d3d12_rhi_tests MK_runtime_host_tests MK_backend_scaffold_tests` passed.
- GREEN focused tests: `ctest --preset dev --output-on-failure -R "MK_rhi_tests|MK_renderer_tests|MK_d3d12_rhi_tests|MK_runtime_host_tests|MK_backend_scaffold_tests"` passed 5/5.
- STATIC: `tools/check-format.ps1`, `tools/check-public-api-boundaries.ps1`, `tools/check-json-contracts.ps1`, `tools/check-agents.ps1`, and `tools/check-ai-integration.ps1` passed after durable guidance and manifest synchronization.
- SLICE GATE: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed; diagnostic-only host gates still report missing Apple/Metal tools on this Windows host as expected.
- PUBLICATION PREFLIGHT: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` passed after validation.
