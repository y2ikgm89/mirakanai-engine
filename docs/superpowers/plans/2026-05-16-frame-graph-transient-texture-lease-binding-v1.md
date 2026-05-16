# Frame Graph Transient Texture Lease Binding v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Bind the existing Frame Graph transient texture alias plan to backend-neutral `IRhiDevice` transient texture leases without claiming native heap aliasing or backend aliasing barriers.

**Architecture:** Keep native heap/resource ownership inside `MK_rhi` backends. `MK_renderer` receives a validated `FrameGraphTransientTextureAliasPlan`, acquires exactly one existing `IRhiDevice::acquire_transient_texture` lease per alias group, and returns both lease rows and `FrameGraphTextureBinding` rows so executor callers can bind each frame-graph resource to the acquired texture handle. On acquisition failure, already acquired leases are released before returning diagnostics. These rows are acquisition output only; alias-aware executor state handoff and native alias barriers remain later slices.

**Tech Stack:** C++23, `MK_renderer`, `MK_rhi` public transient lease contracts, `MK_renderer_tests`, PowerShell 7 validation tools.

---

## Status

**Status:** Completed.

## Official Practice Check

- Microsoft Direct3D 12 documentation describes aliasing barriers as transitions between different resources that map to the same heap. This slice does not create placed resources, map multiple native resources into one heap, or emit native aliasing barriers.
- Existing `IRhiDevice::acquire_transient_texture` is a backend-neutral lease API. This slice binds frame-graph alias groups to that existing lease surface as a foundation for a later native allocator slice.

## Context

`Frame Graph Transient Texture Alias Planning v1` already validates used transient texture descriptors, computes lifetimes, groups non-overlapping exact descriptor matches, and reports byte estimates. No executor-facing helper currently turns those alias groups into texture bindings or ensures leases are released on partial acquisition failure.

## Constraints

- Do not expose native handles, heaps, memory objects, queue objects, or backend barrier structs.
- Do not claim native D3D12 heap aliasing, placed resources, Vulkan/Metal memory aliasing, or backend aliasing barriers.
- Acquire one transient texture lease per alias group, not one per resource in that group.
- Return one `FrameGraphTextureBinding` per resource in each alias group, all starting at `rhi::ResourceState::undefined`.
- Propagate pre-existing plan diagnostics without acquiring anything.
- Accept empty alias plans without acquisition and reject malformed alias groups before acquisition.
- Release already acquired leases if a later alias group acquisition throws.
- Do not claim the current RHI texture executor is alias-aware for multiple resource binding rows that share one texture handle.
- Keep `frame-graph-v1` open and foundation-only.

## Done When

- `frame_graph_rhi.hpp` exposes a small value contract for transient texture lease binding.
- `acquire_frame_graph_transient_texture_lease_bindings` returns deterministic lease rows and texture bindings from a successful alias plan.
- `release_frame_graph_transient_texture_lease_bindings` releases the distinct acquired leases once.
- `MK_renderer_tests` covers empty plans, shared handles for same-group resources, pre-existing plan diagnostics, malformed alias groups, and cleanup after partial acquisition failure.
- Docs, manifest fragments, registry pointers, rendering skills, and static guards distinguish this lease-binding foundation from native heap alias execution.
- Focused tests, agent/static checks, `tools/validate.ps1`, and `tools/build.ps1` pass or record a concrete environment blocker.

## Tasks

### Task 1: RED tests and static guard

**Files:**

- Modify: `tests/unit/renderer_rhi_tests.cpp`
- Modify: `tools/check-ai-integration-030-runtime-rendering.ps1`

- [x] Add tests for the intended public API:
  - empty alias plan succeeds without acquisition
  - successful alias plan acquisition returns one lease per alias group and one binding per aliased resource
  - resources in the same alias group share the same `TextureHandle`
  - pre-existing plan diagnostics are returned without transient acquisitions
  - malformed alias groups are rejected before acquisition
  - partial acquisition failure releases previously acquired leases and reports the failing group
- [x] Add static guards for `FrameGraphTransientTextureLeaseBindingResult`, `acquire_frame_graph_transient_texture_lease_bindings`, and `release_frame_graph_transient_texture_lease_bindings`.
- [x] Run focused `MK_renderer_tests`.

Expected: compile/static failure before implementation because the new API does not exist.

### Task 2: Implement transient texture lease binding

**Files:**

- Modify: `engine/renderer/include/mirakana/renderer/frame_graph_rhi.hpp`
- Modify: `engine/renderer/src/frame_graph_rhi.cpp`

- [x] Add `FrameGraphTransientTextureLease` and `FrameGraphTransientTextureLeaseBindingResult`.
- [x] Implement `acquire_frame_graph_transient_texture_lease_bindings(rhi::IRhiDevice&, const FrameGraphTransientTextureAliasPlan&)`.
- [x] If `plan.diagnostics` is non-empty, copy them to the result and return without acquiring.
- [x] For each alias group, call `device.acquire_transient_texture(group.desc)`, retain the lease, and emit `FrameGraphTextureBinding` rows for every `group.resources` entry using the acquired texture handle and `ResourceState::undefined`.
- [x] Catch `std::exception` from acquisition, release already acquired leases, and return an `invalid_resource` diagnostic that includes the failing alias group index and exception message.
- [x] Implement `release_frame_graph_transient_texture_lease_bindings(rhi::IRhiDevice&, std::span<const FrameGraphTransientTextureLease>)`.
- [x] Run focused `MK_renderer_tests`.

Expected: focused tests pass.

### Task 3: Agent surface and documentation sync

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

- [x] State that frame graph transient texture alias groups can now be acquired as backend-neutral RHI transient leases.
- [x] Keep alias-aware executor state execution, native heap alias execution, backend aliasing barriers, allocator enforcement, multi-queue scheduling, package streaming, Metal parity, and broad renderer quality unsupported.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`.

### Task 4: Validation and publication

**Files:**

- All touched files.

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [ ] Commit, push, create PR, inspect PR state/checks, and register auto-merge only after required preflight is clean.

## Validation Evidence

- RED: focused `MK_renderer_tests` failed to compile before implementation because `acquire_frame_graph_transient_texture_lease_bindings` / `release_frame_graph_transient_texture_lease_bindings` were not defined; `check-ai-integration.ps1` also failed before the manifest/static guard text existed.
- RED: after adding malformed alias-group coverage, focused `MK_renderer_tests` failed because empty alias groups still acquired successfully.
- GREEN: focused `MK_renderer_tests` build plus `ctest --preset dev --output-on-failure -R "MK_renderer_tests"` passed after implementation, malformed-group validation, and empty-plan coverage.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`; CTest reported 65/65 tests passed. Metal/Apple diagnostics remain host-gated as expected on this Windows host.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
