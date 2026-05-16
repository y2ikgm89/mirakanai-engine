# Frame Graph Shared Texture State Handoff v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make Frame Graph RHI texture barrier/pass-target/final-state execution track state by shared `TextureHandle` when multiple frame-graph resource binding rows intentionally refer to the same backend-neutral transient texture lease.

**Architecture:** Keep `FrameGraphTextureBinding` as the public resource-name binding row. The executor remains backend-neutral, but when two or more binding rows share one `rhi::TextureHandle`, a successful transition for one row updates the current state of every row with that handle. This lets lease-bound non-overlapping aliases move through the existing executor without issuing stale `before` states. Native aliasing barriers, placed resources, heap allocation, and overlap validation remain separate backend/executor slices.

**Tech Stack:** C++23, `MK_renderer`, `MK_rhi` public resource states, `MK_renderer_tests`, PowerShell 7 validation tools.

---

## Status

**Status:** Completed.

## Official Practice Check

- Microsoft Direct3D 12 documentation says applications manage resource states explicitly with resource barriers, and aliasing barriers describe transitions between different resources that have mappings into the same heap.
- This slice only keeps backend-neutral state tracking coherent for a single existing `TextureHandle` shared across resource binding rows. It does not create D3D12 placed resources, map multiple native resources into one heap, or emit native aliasing barriers.

## Context

`Frame Graph Transient Texture Lease Binding v1` can return multiple `FrameGraphTextureBinding` rows that share one texture handle for resources in the same alias group. The current executor stores `current_state` per binding row, while `IRhiDevice` tracks state per texture handle. That can produce stale transition `before` states when a later alias reuses the same handle after an earlier alias changed it.

## Constraints

- Do not expose native handles, heaps, memory objects, queue objects, or backend barrier structs.
- Do not claim native D3D12/Vulkan/Metal alias execution, native aliasing barriers, or placed-resource heap aliasing.
- Keep public APIs minimal; prefer internal executor state propagation over a new broad abstraction.
- Reject conflicting initial states for binding rows that share one texture handle.
- Preserve existing behavior for unique texture handles.
- Keep `frame-graph-v1` open and foundation-only.

## Done When

- `record_frame_graph_texture_barriers` updates every binding row that shares the transitioned texture handle.
- `execute_frame_graph_rhi_texture_schedule` uses shared-handle state propagation for scheduled barriers, pass-target state preparation, and final-state restoration.
- `MK_renderer_tests` covers shared-handle barrier propagation, conflicting initial states, and a non-overlapping alias handoff through pass-target/barrier execution.
- Docs, manifest fragments, registry pointers, rendering skills/subagents, and static guards distinguish shared-handle state handoff from native heap alias execution.
- Focused tests, agent/static checks, `tools/validate.ps1`, and `tools/build.ps1` pass or record a concrete environment blocker.

## Tasks

### Task 1: RED tests and static guard

**Files:**

- Modify: `tests/unit/renderer_rhi_tests.cpp`
- Modify: `tools/check-ai-integration-030-runtime-rendering.ps1`

- [x] Add tests for:
  - `record_frame_graph_texture_barriers` propagating one handle transition to all binding rows that share the handle
  - conflicting initial states for the same texture handle being rejected before recording
  - `execute_frame_graph_rhi_texture_schedule` transitioning a later alias from the actual shared handle state after an earlier alias changed it
- [x] Add static guards for the shared-handle state propagation helper/diagnostic text.
- [x] Run focused `MK_renderer_tests`.

Expected: at least the propagation/alias handoff assertions fail before implementation.

### Task 2: Implement shared-handle state handoff

**Files:**

- Modify: `engine/renderer/src/frame_graph_rhi.cpp`

- [x] Add internal shared-handle state validation for `FrameGraphTextureBinding` rows.
- [x] When planning/simulating barriers, propagate simulated states to all binding rows with the same texture handle.
- [x] When recording barriers, pass-target states, and final states, transition from the current shared handle state and update all rows sharing that handle after success.
- [x] Preserve failure behavior: do not mutate binding rows after a failed RHI transition.
- [x] Run focused `MK_renderer_tests`.

Expected: focused tests pass.

### Task 3: Agent surface and documentation sync

**Files:**

- Modify: `docs/rhi.md`
- Modify: `docs/architecture.md`
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

- [x] State that executor state tracking is now shared-handle aware for existing backend-neutral texture handles.
- [x] Keep native heap alias execution, backend aliasing barriers, allocator enforcement, multi-queue scheduling, package streaming, Metal parity, and broad renderer quality unsupported.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`.

### Task 4: Validation and publication

**Files:** all touched files.

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [ ] Commit, push, create PR, inspect PR state/checks, and register auto-merge only after required preflight is clean.

## Validation Evidence

- RED: focused `MK_renderer_tests` failed before implementation on the new shared-handle propagation, conflicting initial shared-handle state, and alias handoff assertions.
- Focused GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_renderer_tests; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "MK_renderer_tests"'` passed.
- Static and agent checks: `tools/format.ps1`, `tools/check-format.ps1`, `tools/check-json-contracts.ps1`, `tools/check-agents.ps1`, and `tools/check-ai-integration.ps1` passed.
- Slice gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed, including 65/65 CTest tests. Metal/Apple checks remain diagnostic host gates on this Windows host.
- Build gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` passed.
