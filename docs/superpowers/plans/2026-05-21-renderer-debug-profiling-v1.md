# Renderer Debug Profiling v1 (2026-05-21)

**Plan ID:** `renderer-debug-profiling-v1`
**Status:** Active.
**Current pointer rule:** Set `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` to this plan while the milestone is active. Keep `unsupportedProductionGaps = []`; this is renderer 1.x developer-owned capability work, not a reopened Engine 1.0 production gap.

## Goal

Strengthen renderer debugging and performance handoff with backend-neutral GPU timestamp/marker rows, capture request/evidence counters, frame diagnostics, and operator handoff before broader production profiling claims.

## Context

- Engine 1.0 closeout remains manifest-led and currently has `unsupportedProductionGaps = []`.
- `renderer-gpu-memory-v1` completed with backend-neutral GPU memory policy diagnostics, package-visible GPU memory counters, and selected D3D12 GPU memory execution evidence.
- The renderer advanced production track lists `renderer-debug-profiling-v1` after GPU memory.
- Official documentation for this milestone will include Microsoft PIX on Windows, Vulkan debug utils / validation guidance, and Apple Metal capture / Xcode / Instruments guidance.

## Constraints

- Preserve `unsupportedProductionGaps = []`.
- Keep public renderer/game APIs backend-neutral; native capture tools, queues, heaps, and profiler handles stay backend-private.
- Do not claim crash telemetry, symbol-server operations, production flame graphs, or automatic external tool execution from editor core without separate plans.
- Start behavior/API/regression-risk changes with a RED test or static guard.

## Phase 0: Pointer Sync

**Status:** Pending.

### Goal

Close the completed GPU memory active pointer and select `renderer-debug-profiling-v1` as the next active developer-owned renderer 1.x capability.

### Done When

- `docs/superpowers/plans/README.md` lists this plan as the active milestone and records `renderer-gpu-memory-v1` as completed.
- Master plan, readiness ledger, and `docs/roadmap.md` identify this plan as active without reopening production gaps.
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` and composed `engine/agent/manifest.json` point `currentActivePlan` and `recommendedNextPlan` at this plan while preserving `unsupportedProductionGaps = []`.
- Static JSON/agent integration checks pass for the pointer transition.

## Validation Evidence

- Phase 0 pending until pointer sync lands after `renderer-gpu-memory-v1` closeout.
