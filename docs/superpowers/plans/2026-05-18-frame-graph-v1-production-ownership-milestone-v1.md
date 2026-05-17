# Frame Graph v1 Production Ownership Milestone v1 (2026-05-18)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Complete the next `frame-graph-v1` foundation milestone by moving from package-visible executor evidence to production graph ownership boundaries without broadening Vulkan/Metal aliasing, async performance, broad streaming, native-handle, or broad renderer-quality claims.

**Architecture:** Keep gameplay and runtime callers on first-party Frame Graph/RHI contracts. Work in phase gates over production graph ownership beyond runtime uploads, package/resource upload integration boundaries, and production multi-queue adoption. Keep backend-specific memory alias allocation, async/performance proof, broad/background streaming, and renderer-quality promotion split unless the same host gate and review boundary can prove them.

**Tech Stack:** C++23 MIRAIKANAI renderer/RHI/runtime package contracts, Frame Graph v1, `MK_renderer`, `MK_runtime_rhi`, `MK_rhi`, PowerShell validation wrappers, composed engine agent manifest.

---

**Plan ID:** `frame-graph-v1-production-ownership-milestone-v1`
**Status:** Active.
**Parent:** [Production Completion Master Plan v1](2026-05-03-production-completion-master-plan-v1.md)
**Gap:** `frame-graph-v1`

## Context

- The previous `frame-graph-v1` slices proved executor-owned texture transitions, pass target-state rows, render-pass envelopes, package-visible render-pass counters, backend-neutral queue-wait planning, multi-queue command-list envelopes, package-visible multi-queue evidence, runtime texture uploads, and runtime mesh/skinned/morph upload command evidence.
- The gap still does not claim production graph ownership beyond selected renderer/runtime-upload paths, production multi-queue graph adoption, upload/staging package integration, Vulkan/Metal memory alias allocation, async overlap/performance, broad/background package streaming, public native handles, or broad renderer quality.
- This milestone is intentionally wider than the previous one-function slices. Each phase should still be one reviewable PR/checkpoint with focused validation and a clear ready-claim boundary.

## Constraints

- Do not expose native RHI, queue, fence, semaphore, heap, or descriptor handles through gameplay/runtime package APIs.
- Do not claim Vulkan/Metal memory alias allocation from Windows D3D12 evidence.
- Do not claim async overlap/performance from package-visible multi-queue counters without measured backend evidence.
- Do not add broad/background package streaming, staging rings, allocator-budget enforcement, or renderer-quality readiness unless a phase explicitly accepts and validates that scope.
- Keep clean-break implementation choices; do not add compatibility shims or deprecated aliases.
- Use independent subagents for sidecar audits/reviews/failure triage or disjoint implementation when user-authorized; keep blocker decisions, final integration, validation evidence, git, push, and PR ownership in the parent session.

## Phase 1: Production Graph Ownership Boundary

**Goal:** Select the next production renderer/runtime boundary and add RED tests for the externally meaningful behavior before changing implementation.

**Likely files to inspect before editing:**
- `engine/renderer/src/frame_graph_rhi.cpp`
- `engine/renderer/src/rhi_frame_renderer.cpp`
- `engine/renderer/src/rhi_postprocess_frame_renderer.cpp`
- `engine/renderer/src/rhi_directional_shadow_smoke_frame_renderer.cpp`
- `engine/runtime_rhi/src/runtime_upload.cpp`
- `tests/unit/*frame*`

- [ ] Identify the smallest production graph ownership boundary still outside the executor after the runtime upload command-evidence slices.
- [ ] Record the accepted non-goals for this phase before implementation.
- [ ] Add RED tests for the selected renderer/runtime behavior and prove they fail for the right reason.
- [ ] Ask a read-only rendering/RHI subagent to audit the selected boundary while local work continues on tests or implementation.

## Phase 2: Renderer Production Graph Ownership

**Goal:** Move the selected renderer-owned graph/pass setup into the Frame Graph executor while keeping swapchain acquire/present/readback and host-visible submission ownership explicit.

- [ ] Implement the selected production renderer ownership change through first-party Frame Graph/RHI APIs.
- [ ] Preserve existing package-visible counters and diagnostics unless the phase intentionally changes them.
- [ ] Run focused build/test/tidy loops for the touched renderer/runtime files.
- [ ] Use reviewer/auditor subagents for ownership, lifetime, and renderer-synchronization review before closeout.

## Phase 3: Package/Resource Upload Integration Boundary

**Goal:** Route the next package/resource upload integration boundary through the existing executor without turning this milestone into broad streaming or staging-ring work.

- [ ] Select the narrow package/resource upload handoff that directly depends on completed runtime upload and package-visible Frame Graph evidence.
- [ ] Keep broad/background streaming, staging rings, allocator budgets, and renderer-owned residency out of scope unless a later phase explicitly accepts them.
- [ ] Prove the handoff with package-visible or runtime-visible evidence, not only internal plumbing.

## Phase 4: Production Multi-Queue Adoption Proof

**Goal:** Promote the completed package-visible multi-queue executor evidence into one selected production graph path if the same API and validation surface can prove it.

- [ ] Select a production graph path whose queue ownership can be validated without public native queue/fence exposure.
- [ ] Reuse existing `FrameGraphRhiMultiQueueExecutionDesc` / result evidence where possible.
- [ ] Keep async overlap/performance claims blocked until measured backend evidence exists.

## Phase 5: Closeout Sync

**Goal:** Either close `frame-graph-v1`, record a host-gated blocker, or narrow the next milestone with evidence.

- [ ] Update manifest fragments and compose `engine/agent/manifest.json`.
- [ ] Update the plan registry, master plan, roadmap/current capabilities, and affected skills/rules/subagents only where durable guidance or AI-operable contracts changed.
- [ ] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.
- [ ] Run focused agent/static checks and one fresh `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` for any C++/runtime/build/public-contract closeout phase.
- [ ] Publish the phase through GitHub Flow: validated commit, non-forced push, focused PR update or draft PR when work is still in progress.

## Validation Cadence

| Work type | Expected checks |
| --- | --- |
| Docs/manifest planning only | `git diff --check`, `tools/check-format.ps1`, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-agents.ps1`, `tools/check-production-readiness-audit.ps1` when gap rows or production-loop pointers changed. |
| C++ focused phase | `tools/check-toolchain.ps1`, `tools/cmake.ps1 --preset dev`, targeted `tools/cmake.ps1 --build --preset dev --target <target>`, targeted `tools/ctest.ps1 --preset dev --output-on-failure -R <test>`, `tools/check-tidy.ps1 -Files ...`, plus relevant agent/static checks. |
| Public API/backend interop phase | Add `tools/check-public-api-boundaries.ps1` and official-source notes for touched SDK/toolchain behavior. |
| Coherent phase close | One fresh `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` after code, docs, manifest, and static checks settle. |
| Gap closeout | `tools/check-production-readiness-audit.ps1`, manifest/static sync, roadmap/current capability sync, and full `tools/validate.ps1` unless a concrete host/tool blocker is recorded. |

## Validation Evidence

| Command | Status | Notes |
| --- | --- | --- |
| Pending | Pending | Fill this table as phases close; do not claim phase completion without command output or a concrete host/tool blocker. |
