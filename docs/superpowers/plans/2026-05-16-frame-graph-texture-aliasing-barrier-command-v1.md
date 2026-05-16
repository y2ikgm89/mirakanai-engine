# Frame Graph Texture Aliasing Barrier Command v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a backend-neutral texture aliasing barrier command contract that Frame Graph helpers can record before future native transient heap alias allocation is implemented.

**Architecture:** Add an explicit `IRhiCommandList` texture aliasing barrier command that records a dependency between two distinct backend-neutral texture handles without changing their tracked resource states. `MK_renderer` adds a small Frame Graph recording helper over resource-name bindings so alias barrier intent can be validated and recorded deterministically. D3D12 validates committed texture handles and records the engine-level aliasing intent/stat without emitting a native placed-resource aliasing barrier in this slice; Vulkan records a conservative synchronization2 memory dependency; Null RHI validates/counts the command. Native placed resources, shared heap allocation, memory-object alias planning, wildcard/null aliasing barriers, automatic executor insertion, package streaming, and production graph ownership remain separate slices.

**Tech Stack:** C++23, `MK_rhi`, D3D12 committed-resource validation plus engine-level aliasing intent counters, Vulkan synchronization2 memory barriers, `MK_renderer`, PowerShell 7 validation tools.

---

## Status

**Status:** Completed.

## Official Practice Check

- Microsoft Direct3D 12 documentation defines aliasing barriers as resource barriers for transitions between usages of two different resources that share the same heap mapping.
- Current D3D12 `DeviceContext` textures are committed resources, so this slice deliberately avoids native non-null or wildcard/null aliasing barrier emission until placed/reserved resource provenance exists.
- Vulkan synchronization guidance requires explicit memory dependencies for hazards that are not covered by layout-specific image transitions. This slice uses a conservative memory barrier for backend-neutral aliasing dependency evidence and does not claim native Vulkan memory alias allocation.

## Context

Frame Graph v1 already has transient texture alias planning, backend-neutral transient texture lease binding, and shared-`TextureHandle` state handoff for rows sharing one existing lease texture. The remaining `frame-graph-v1` production gap still lists native transient heap allocation/alias execution and native aliasing barriers as unsupported. Before allocator work can be honest, the RHI command surface needs a first-party aliasing barrier contract that can be tested independently from native heap allocation.

## Constraints

- Do not expose native D3D12 resources, Vulkan images/memory objects, heaps, command buffers, or backend barrier structs in public engine APIs.
- Do not allocate native placed resources, shared heaps, Vulkan aliasing memory, or Metal heaps in this slice.
- Do not insert aliasing barriers automatically into `execute_frame_graph_rhi_texture_schedule` yet; keep this as an explicit recording helper for future allocator/executor integration.
- Require two distinct non-empty texture handles for the public command and helper; wildcard/null aliasing barriers can be designed later if needed.
- Keep tracked resource states unchanged by aliasing barriers.
- Keep `frame-graph-v1` open and foundation-only.

## Done When

- `IRhiCommandList` exposes a texture aliasing barrier command with Null, D3D12, and Vulkan implementations.
- `RhiStats` and D3D12 `DeviceContextStats` expose a deterministic aliasing barrier counter.
- `MK_renderer` exposes `FrameGraphTextureAliasingBarrier`, a record result, and `record_frame_graph_texture_aliasing_barriers`.
- Tests cover successful Null/D3D12/Vulkan command recording where host support is available, rejection of invalid handles/same handles/closed command lists, deterministic Frame Graph helper diagnostics, and unchanged texture states.
- Docs, plans, manifest fragments, rendering skills, rendering auditor guidance, and static guards distinguish aliasing barrier command support from native heap alias allocation and automatic executor integration.
- Focused RHI/renderer tests, agent/static checks, `tools/validate.ps1`, and `tools/build.ps1` pass or record a concrete environment blocker.

## Tasks

### Task 1: RED tests and public contract guards

**Files:**

- Modify: `engine/rhi/include/mirakana/rhi/rhi.hpp`
- Modify: `engine/renderer/include/mirakana/renderer/frame_graph_rhi.hpp`
- Modify: `tests/unit/rhi_tests.cpp`
- Modify: `tests/unit/d3d12_rhi_tests.cpp`
- Modify: `tests/unit/renderer_rhi_tests.cpp`
- Modify: `tools/check-ai-integration-030-runtime-rendering.ps1`

- [x] Add RED tests for `IRhiCommandList::texture_aliasing_barrier` on `NullRhiDevice`: records outside a render pass, increments a dedicated counter, leaves both texture states unchanged, and rejects same/empty/unknown texture handles.
- [x] Add D3D12 host test that records committed texture aliasing intent on a graphics command list and checks both `RhiStats` and `DeviceContextStats` counters without claiming placed-resource alias allocation.
- [x] Add Vulkan host-gated or environment-gated test coverage for conservative memory-barrier recording when the Vulkan runtime path is available.
- [x] Add Frame Graph helper RED tests for resource-name-to-texture aliasing barrier recording, missing resources, same-handle rejection, and closed command-list rejection.
- [x] Add static guards for the new public API names and tests.
- [x] Run focused `MK_rhi_tests`, `MK_d3d12_rhi_tests`, and `MK_renderer_tests`.

Expected: the new tests fail to compile before implementation because the public API and helper types do not exist.

### Task 2: Implement RHI aliasing barrier command

**Files:**

- Modify: `engine/rhi/include/mirakana/rhi/rhi.hpp`
- Modify: `engine/rhi/src/null_rhi.cpp`
- Modify: `engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp`
- Modify: `engine/rhi/d3d12/src/d3d12_backend.cpp`
- Modify: `engine/rhi/vulkan/src/vulkan_backend.cpp`
- Modify: `tests/unit/renderer_rhi_tests.cpp`

- [x] Add `texture_aliasing_barrier(TextureHandle before, TextureHandle after)` to `IRhiCommandList`.
- [x] Add `texture_aliasing_barriers` to `RhiStats` and D3D12 `DeviceContextStats`.
- [x] Implement Null validation/counting without mutating texture states.
- [x] Implement D3D12 committed texture validation and engine-level aliasing intent/stat recording without native placed-resource alias barrier emission.
- [x] Implement Vulkan conservative synchronization2 memory barrier recording.
- [x] Update all test command-list wrappers to forward or deliberately throw through the new method.
- [x] Run focused RHI tests.

Expected: focused RHI tests pass.

### Task 3: Implement Frame Graph recording helper

**Files:**

- Modify: `engine/renderer/include/mirakana/renderer/frame_graph_rhi.hpp`
- Modify: `engine/renderer/src/frame_graph_rhi.cpp`
- Modify: `tests/unit/renderer_rhi_tests.cpp`

- [x] Add `FrameGraphTextureAliasingBarrier` and `FrameGraphTextureAliasingBarrierRecordResult`.
- [x] Implement `record_frame_graph_texture_aliasing_barriers(rhi::IRhiCommandList&, std::span<const FrameGraphTextureAliasingBarrier>, std::span<const FrameGraphTextureBinding>)`.
- [x] Validate resource names, missing bindings, same texture handles, and closed command lists before recording each barrier.
- [x] Convert RHI exceptions to deterministic `FrameGraphDiagnostic` rows and stop after failure.
- [x] Run focused `MK_renderer_tests`.

Expected: focused renderer tests pass.

### Task 4: Documentation, manifest, and agent surfaces

**Files:**

- Modify: `docs/rhi.md`
- Modify: `docs/architecture.md`
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
- Modify: `tools/check-ai-integration-020-engine-manifest.ps1`
- Modify: `tools/check-ai-integration-030-runtime-rendering.ps1`
- Modify: `tools/check-json-contracts-040-agent-surfaces.ps1`

- [x] State that aliasing barrier command recording exists as a narrow RHI/Frame Graph foundation.
- [x] Keep native transient heap allocation, native placed resources, automatic executor insertion, wildcard aliasing barriers, multi-queue scheduling, package streaming, Metal parity, and broad renderer quality unsupported.
- [x] Update `currentActivePlan`, `recommendedNextPlan`, and the `frame-graph-v1` gap notes in `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`.

### Task 5: Validation and publication

**Files:** all touched files.

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run focused CMake/CTest targets for RHI and renderer tests.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Commit, push, create PR, inspect PR state/checks, and register auto-merge only after required preflight is clean.

## Validation Evidence

- RED evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_rhi_tests MK_renderer_tests MK_backend_scaffold_tests MK_d3d12_rhi_tests'` failed before implementation because `IRhiCommandList::texture_aliasing_barrier` and `RhiStats::texture_aliasing_barriers` did not exist.
- Focused failure evidence: the first implementation run of `MK_rhi_tests`, `MK_renderer_tests`, `MK_backend_scaffold_tests`, and `MK_d3d12_rhi_tests` exposed an invalid NullRHI initial-state assumption and an over-broad D3D12 native aliasing-barrier emission on committed textures.
- Focused GREEN evidence: after narrowing D3D12 to committed-resource intent/stat recording, `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_rhi_tests MK_renderer_tests MK_backend_scaffold_tests MK_d3d12_rhi_tests; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "MK_rhi_tests|MK_renderer_tests|MK_backend_scaffold_tests|MK_d3d12_rhi_tests"'` passed: 4/4 tests passed. Existing MSB8028 shared-intermediate-directory warnings were emitted by MSBuild.
- Manifest compose evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` passed and regenerated `engine/agent/manifest.json`.
- Formatting evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` passed after normalizing one tracked text file.
- Static evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed.
- Static evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed.
- Static evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed after manifest `recommendedNextPlan` wording and alias-barrier contract guards were synchronized.
- Static evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` passed after rendering-auditor initial-load guidance was shortened under the enforced byte budget.
- Static evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after runtime-rendering, manifest, and agent-surface needles were updated.
- Full validation evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` exited 0.
- Full build evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` exited 0. Existing MSB8028 shared-intermediate-directory warnings were emitted by MSBuild.
- Publication evidence: committed `9ab6fee441125c7d737298cf64b2bbe684d4a19a`, pushed `codex-frame-graph-texture-aliasing-barrier-command`, and opened PR `https://github.com/y2ikgm89/mirakanai-engine/pull/69` against `main`.
- PR preflight evidence: `gh pr view 69 --json state,isDraft,baseRefName,headRefName,headRefOid,mergeable,mergeStateStatus,reviewDecision,statusCheckRollup,url` reported `OPEN`, `isDraft=false`, `baseRefName=main`, `mergeable=MERGEABLE`, and `mergeStateStatus=UNSTABLE` with CodeQL, iOS smoke, and validation-tier checks still in progress. Auto-merge was not registered while checks were still running.
