# Frame Graph D3D12 Overlapping Placed Texture Alias Execution v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Prove D3D12 backend-private overlapping placed texture alias barrier recording and submit-time state bookkeeping with distinct resource handles and non-null aliasing barriers.

**Architecture:** Add a D3D12 `DeviceContext` alias-group allocation path that creates one heap and multiple placed textures at offset `0` for exact-desc transient texture aliases. Keep the public RHI/native-handle boundary intact: the proof stays backend-private, `IRhiCommandList::texture_aliasing_barrier` records non-null D3D12 aliasing barriers only for proven same alias-group placed aliases, and committed, standalone placed, or otherwise unproven texture pairs continue to use conservative null-resource aliasing barriers. Automatic frame-graph executor insertion, backend-neutral alias-group leases, Vulkan/Metal memory alias allocation, GPU-visible data inheritance/content preservation, and clear/copy/readback proof remain out of scope.

**Tech Stack:** C++23, `MK_rhi_d3d12`, D3D12 `CreateHeap`, `CreatePlacedResource`, `D3D12_RESOURCE_ALIASING_BARRIER`, PowerShell 7 validation tools.

---

## Status

**Status:** Completed.

## Official Practice Check

- Microsoft Learn documents `D3D12_RESOURCE_ALIASING_BARRIER` as the transition between usages of two different resources that map into the same heap.
- Microsoft Learn resource-barrier synchronization guidance says aliasing barriers are required when two resources with overlapping mappings transition between usages; `pResourceBefore` and `pResourceAfter` may be null only for broad potential aliasing, which this repository keeps backend-private rather than public.
- Microsoft Learn memory aliasing and data-inheritance guidance says a barrier is required between resources sharing physical memory even without data inheritance, and invalidated resources must be reinitialized by clear/copy/full writes under the documented constraints before relying on contents. This slice records barrier/state evidence only and does not claim content inheritance or post-switch texture contents.
- Source anchors: `https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_resource_aliasing_barrier`, `https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12`, and `https://learn.microsoft.com/en-us/windows/win32/direct3d12/memory-aliasing-and-data-inheritance`.

## Context

The previous D3D12 placed transient texture lease slice creates one private heap and one placed texture per transient lease, then activates first command-list use with a null-before/non-null-after aliasing barrier. Frame Graph alias planning and lease binding still share one backend-neutral `TextureHandle` per alias group, so explicit frame graph aliasing barriers reject those shared handles. The next foundation step is backend-private D3D12 evidence that distinct placed texture handles can overlap one heap and switch ownership through non-null `D3D12_RESOURCE_ALIASING_BARRIER` rows.

## Constraints

- Do not expose D3D12 heaps, resources, descriptors, offsets, or native handles through public RHI/gameplay APIs.
- Do not add automatic `execute_frame_graph_rhi_texture_schedule` aliasing-barrier insertion.
- Do not change `FrameGraphTransientTextureLeaseBindingResult` to return distinct public handles in this slice.
- Do not claim data inheritance, contents preservation, post-switch texture contents, Vulkan/Metal alias memory allocation, package streaming integration, multi-queue graph scheduling, or broad renderer readiness.
- Keep committed-resource and unrelated placed-resource aliasing barriers conservative and backend-private null-resource based.

## Done When

- D3D12 device-context tests prove one alias-group heap can own multiple placed texture handles at the same offset, with distinct handles, one heap allocation, multiple placed resource records, and correct lifetime counters.
- D3D12 device-context tests prove `texture_aliasing_barrier(before, after)` records a non-null placed-resource aliasing barrier only when both handles are proven same alias-group placed aliases; standalone placed texture pairs stay on the conservative null-resource path.
- First-use placed-resource activation remains command-list pending until submit; aliasing barriers update active/inactive placed-resource state after `ExecuteCommandLists` hands work to the queue, so reset-before-submit re-record behavior stays correct and post-submit state does not wait for fence completion.
- Partial alias destruction preserves the sibling placed resource/heap owner and keeps lifetime counters honest.
- Docs, plan registry, manifest fragments, skills/subagents, and static integration guards distinguish backend-private D3D12 overlapping alias execution from public wildcard/null barriers, automatic executor insertion, backend-neutral alias-group leases, Vulkan/Metal memory aliasing, data inheritance, and broad renderer readiness.
- Focused D3D12/RHI tests, relevant static/agent checks, `tools/validate.ps1`, and `tools/build.ps1` pass or record a concrete environment blocker.

## Tasks

### Task 1: RED D3D12 alias group tests

**Files:**

- Modify: `engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp`
- Modify: `tests/unit/d3d12_rhi_tests.cpp`

- [x] Add `DeviceContextStats::placed_texture_alias_groups_created` and `DeviceContextStats::placed_resource_aliasing_barriers` test expectations.
- [x] Add `DeviceContext::create_placed_texture_alias_group(TextureDesc, std::size_t)` test coverage for two exact-desc alias handles sharing one alias group and one heap at offset `0`.
- [x] Assert the two handles are distinct, `placed_texture_heaps_created == 1`, `placed_textures_created == 2`, `placed_resources_alive == 2`, and `committed_textures_created == 0`; also assert standalone placed texture pairs keep using conservative null-resource aliasing barriers.
- [x] Activate the first alias, submit it, then record `texture_aliasing_barrier(first, second)` and assert `texture_aliasing_barriers == 1`, `placed_resource_aliasing_barriers == 1`, and `null_resource_aliasing_barriers == 0`.
- [x] Submit the alias barrier and assert an immediate post-submit, pre-fence `activate_placed_texture(second)` does not record a null activation barrier.
- [x] Record a second non-null aliasing barrier from `second` back to `first`, submit it, and assert `placed_resource_aliasing_barriers == 2`.
- [x] Destroy one alias and assert the sibling can still be activated before final destruction drops `placed_resources_alive` to `0`.
- [x] Confirm focused `MK_d3d12_rhi_tests` fails before implementation because the new API/stat fields do not exist.

### Task 2: D3D12 backend-private alias group implementation

**Files:**

- Modify: `engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp`
- Modify: `engine/rhi/d3d12/src/d3d12_backend.cpp`
- Modify: `tests/unit/d3d12_rhi_tests.cpp`

- [x] Add backend-private stats for alias groups and non-null placed-resource aliasing barriers.
- [x] Add `DeviceContext::create_placed_texture_alias_group` that validates the texture desc and `texture_count >= 2`, creates one default heap sized with `GetResourceAllocationInfo`, creates each placed texture at offset `0`, and retains the same heap `ComPtr` for every returned resource handle.
- [x] Track command-list pending placed-resource activation as ordered before/after records so null activation and non-null alias switching update placed active state only after `execute_command_list` submits work with `ExecuteCommandLists`; if a later queue `Signal` fails, do not leave stale pending state that suppresses required future barriers.
- [x] Make `DeviceContext::texture_aliasing_barrier` record a non-null `D3D12_RESOURCE_BARRIER_TYPE_ALIASING` only when both native resources are placed and share the same backend alias-group proof; otherwise preserve the existing conservative null-resource barrier path.
- [x] Keep repeated reset-before-submit activation behavior from the previous slice intact.
- [x] Run focused `MK_d3d12_rhi_tests` to GREEN.

### Task 3: Durable guidance and agent surface

**Files:**

- Modify: `docs/rhi.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `.agents/skills/rendering-change/references/full-guidance.md`
- Modify: `.claude/skills/gameengine-rendering/references/full-guidance.md`
- Modify: `.codex/agents/rendering-auditor.toml`
- Modify: `.claude/agents/rendering-auditor.md`
- Modify: `tools/check-ai-integration-030-runtime-rendering.ps1`

- [x] Record that D3D12 backend-private alias groups can create multiple same-offset placed texture handles with backend alias-group proof and record non-null aliasing barriers.
- [x] Keep backend-neutral frame graph alias group leases, automatic executor insertion, public wildcard/null alias barriers, Vulkan/Metal memory aliasing, data inheritance, package streaming, multi-queue scheduling, and broad renderer readiness unsupported.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` after manifest fragment changes.

### Task 4: Validation and publication

**Files:** all touched files.

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`.
- [x] Run focused `MK_d3d12_rhi_tests`, `MK_rhi_tests`, `MK_renderer_tests`, and `MK_backend_scaffold_tests`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Commit, push, create PR, inspect PR state/checks, and register auto-merge only after required preflight is clean.

## Validation Evidence

- OFFICIAL DOCS: Context7 `/websites/learn_microsoft_en-us_windows_win32_direct3d12` was used to re-check Microsoft Learn Direct3D 12 aliasing barrier and memory aliasing guidance before implementation.
- RED: `cmake --build --preset dev --target MK_d3d12_rhi_tests` failed before implementation because `DeviceContext::create_placed_texture_alias_group`, `DeviceContextStats::placed_texture_alias_groups_created`, and `DeviceContextStats::placed_resource_aliasing_barriers` did not exist.
- FOCUSED BUILD: `cmake --build --preset dev --target MK_d3d12_rhi_tests MK_rhi_tests MK_renderer_tests MK_backend_scaffold_tests` passed.
- FOCUSED TESTS: `ctest --preset dev --output-on-failure -R "MK_d3d12_rhi_tests|MK_rhi_tests|MK_renderer_tests|MK_backend_scaffold_tests"` passed 4/4 tests.
- STATIC: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `tools/check-public-api-boundaries.ps1`, `tools/check-json-contracts.ps1`, `tools/check-agents.ps1`, and `tools/check-ai-integration.ps1` passed.
- FULL VALIDATION: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed, including 65/65 CTest tests. Metal/Apple checks remain host-gated diagnostics on this Windows host as expected.
- BUILD: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` passed after full validation.
- PUBLICATION: Commit `ea5101f` was pushed to `codex/frame-graph-d3d12-overlapping-placed-texture-alias-execution`, and PR `#80` was created at `https://github.com/y2ikgm89/mirakanai-engine/pull/80`.
