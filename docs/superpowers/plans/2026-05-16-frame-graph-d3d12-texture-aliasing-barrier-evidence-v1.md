# Frame Graph D3D12 Texture Aliasing Barrier Evidence v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the D3D12 texture aliasing barrier implementation and durable agent guidance agree by recording a conservative backend-private null-resource D3D12 aliasing barrier after public handle validation.

**Architecture:** Keep the public `IRhiCommandList::texture_aliasing_barrier(TextureHandle before, TextureHandle after)` contract strict: callers must provide two distinct backend-neutral texture handles, and no native D3D12 handles or wildcard/null handles are exposed. Inside the D3D12 backend, validate those public handles as committed textures, then record a backend-private `D3D12_RESOURCE_BARRIER_TYPE_ALIASING` with both native resource pointers null, which Microsoft documents as a conservative aliasing barrier form. This is evidence for explicit barrier command support only; it does not implement placed resources, native transient heap allocation, automatic Frame Graph insertion, or production alias execution.

**Tech Stack:** C++23, `MK_rhi_d3d12`, D3D12 `ResourceBarrier`, PowerShell 7 validation tools.

---

## Status

**Status:** Completed.

## Official Practice Check

- Microsoft `ID3D12GraphicsCommandList::ResourceBarrier` documentation says `D3D12_RESOURCE_ALIASING_BARRIER` describes transitions between usages of resources mapped into the same heap, and that one or both resources can be null to indicate broad possible aliasing.
- Microsoft `CreatePlacedResource` documentation recommends the simple placed-resource aliasing model until advanced usage is needed, and notes that an aliasing barrier can set both `pResourceBefore` and `pResourceAfter` to null.
- This slice records the conservative D3D12 null-resource barrier only after the engine public API validates two distinct texture handles. Public wildcard/null aliasing remains unsupported.

## Context

The previous texture aliasing barrier command slice added `IRhiCommandList::texture_aliasing_barrier`, renderer helper validation, and D3D12 stats. A follow-up audit found a durable-guidance mismatch: repository guidance claimed D3D12 records a conservative null-resource native aliasing barrier, while the implementation only incremented an intent counter. This slice closes that discrepancy before larger native transient heap or placed-resource aliasing work.

## Constraints

- Do not change the public `IRhiCommandList` signature.
- Do not add public native handles, heaps, memory objects, or backend barrier structs.
- Do not add placed resources, shared heaps, Vulkan memory alias allocation, Metal heaps, or native transient heap allocation.
- Do not insert aliasing barriers automatically into `execute_frame_graph_rhi_texture_schedule`.
- Do not add public wildcard/null aliasing barrier calls.
- Keep `frame-graph-v1` open and foundation-only.

## Done When

- D3D12 records one native `D3D12_RESOURCE_BARRIER_TYPE_ALIASING` with null resource pointers after public handle validation.
- `DeviceContextStats` exposes deterministic evidence for backend-private null-resource aliasing barrier recording.
- D3D12 `DeviceContext` and `IRhiDevice` tests prove close/submit/wait remains valid and transition counters stay unchanged.
- Manifest/docs/skills/subagents/static guards distinguish backend-private null-resource D3D12 evidence from public wildcard/null aliasing and placed-resource alias execution.
- Focused RHI/D3D12 tests, agent/static checks, `tools/validate.ps1`, and `tools/build.ps1` pass or record a concrete environment blocker.

## Tasks

### Task 1: RED D3D12 evidence tests

**Files:**

- Modify: `engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp`
- Modify: `tests/unit/d3d12_rhi_tests.cpp`

- [x] Add a `DeviceContextStats::null_resource_aliasing_barriers` assertion to the existing D3D12 device-context aliasing barrier test.
- [x] Confirm the focused D3D12 test build fails before implementation because the new evidence counter does not exist.

### Task 2: D3D12 backend implementation

**Files:**

- Modify: `engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp`
- Modify: `engine/rhi/d3d12/src/d3d12_backend.cpp`
- Modify: `tests/unit/d3d12_rhi_tests.cpp`

- [x] Add `null_resource_aliasing_barriers` to `DeviceContextStats`.
- [x] After existing command-list, queue, resource, and texture validation, record `D3D12_RESOURCE_BARRIER_TYPE_ALIASING` with `Aliasing.pResourceBefore = nullptr` and `Aliasing.pResourceAfter = nullptr`.
- [x] Increment both `texture_aliasing_barriers` and `null_resource_aliasing_barriers`.
- [x] Keep invalid handles, identical handles, buffers, closed lists, and non-graphics queues rejected before any native barrier is recorded.
- [x] Run focused `MK_d3d12_rhi_tests`.

### Task 3: Agent surface and durable guidance

**Files:**

- Modify: `docs/rhi.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `docs/superpowers/plans/2026-05-16-frame-graph-texture-aliasing-barrier-command-v1.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `tools/check-ai-integration-030-runtime-rendering.ps1`
- Review: `.agents/skills/rendering-change/references/full-guidance.md`
- Review: `.claude/skills/gameengine-rendering/references/full-guidance.md`
- Review: `.codex/agents/rendering-auditor.toml`
- Review: `.claude/agents/rendering-auditor.md`

- [x] Update wording from D3D12 intent-only evidence to backend-private null-resource native aliasing barrier evidence.
- [x] Keep public wildcard/null aliasing barriers, D3D12 placed-resource aliasing, native transient heap allocation, automatic executor insertion, package streaming, Metal parity, and broad renderer quality unsupported.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` after manifest fragment changes.

### Task 4: Validation and publication

**Files:** all touched files.

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`.
- [x] Run focused `MK_rhi_tests`, `MK_d3d12_rhi_tests`, `MK_renderer_tests`, and `MK_backend_scaffold_tests`.
- [x] Run `check-format`, `check-public-api-boundaries`, `check-json-contracts`, `check-agents`, and `check-ai-integration`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [ ] Commit, push, create PR, inspect PR state/checks, and register auto-merge only after required preflight is clean.

## Validation Evidence

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_d3d12_rhi_tests'` failed before implementation because `DeviceContextStats::null_resource_aliasing_barriers` did not exist.
- GREEN: focused `MK_d3d12_rhi_tests` build and `ctest --preset dev --output-on-failure -R "MK_d3d12_rhi_tests"` passed `1/1`.
- Focused renderer/RHI loop: `MK_rhi_tests`, `MK_backend_scaffold_tests`, `MK_renderer_tests`, and `MK_d3d12_rhi_tests` built and passed `4/4`.
- Manifest: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` updated the composed manifest from fragments.
- Formatting/static: `tools/format.ps1`, `tools/check-format.ps1`, `tools/check-public-api-boundaries.ps1`, `tools/check-json-contracts.ps1`, `tools/check-agents.ps1`, and `tools/check-ai-integration.ps1` passed. The first `check-ai-integration` run exposed one stale cross-line needle; the guard now checks stable `backend-private`, `null-resource aliasing barrier`, and `D3D12_RESOURCE_BARRIER_TYPE_ALIASING` evidence separately.
- Drift review: Codex/Claude rendering skills and rendering-auditor subagents were searched for aliasing guidance and already distinguished backend-private D3D12 null-resource evidence from unsupported public wildcard/null barriers, native allocation, and placed-resource alias execution, so no skill/subagent text change was needed.
- Slice gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with `65/65` CTest tests passing; host diagnostics continued to report existing Metal/Apple host gates as diagnostic-only or host-gated.
- Commit gate: standalone `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` passed.
