# 2026-06-01 Performance Baseline v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add the first reproducible performance baseline lane so optimization work starts from measured frame, trace, and counter evidence instead of broad CPU/GPU/memory claims.

**Architecture:** Reuse existing `MK_core` diagnostics, Trace Event JSON export/import, runtime package validation, and `game.agent.json.performanceBudgets`. Start with the smallest ready package lane, then add p95/p99 budget reporting and artifact references before changing allocators, schedulers, renderer/RHI queues, vendor-specific kernels, CUDA/HIP, or GPU-driven rendering.

**Tech Stack:** C++23 `MK_core` diagnostics, PowerShell 7 repository wrappers, `tools/validate-installed-desktop-runtime.ps1`, selected `game.agent.json` validation recipes, composed `engine/agent/manifest.json`, `tools/check-json-contracts.ps1`, and `tools/check-ai-integration.ps1`.

---

**Plan ID:** `performance-baseline-v1`

**Status:** Active.

Selected on 2026-06-01 after [AI-Operable Performance Budget And Evidence v1](2026-06-01-ai-operable-performance-budget-and-evidence-v1.md) completed the descriptor/schema layer through PR #361.

## Optimization Ordering Decision

Do the measurement and contract work before broad implementation:

- Required first: reproducible package scenes, deterministic run settings, trace export recipes, subsystem counters, p95/p99 frame budget reporting, and AI-readable evidence rows.
- Safe early reviews: memory lifetime taxonomy, pointer/handle semantics audit, bitmask bounds, and vendor profiler matrix definitions.
- Delay until evidence exists: all-core scheduling policy, worker/job graph execution, allocator replacement, NUMA/affinity tuning, SIMD dispatch, GPU queue overlap, CUDA/HIP adapters, GPU-driven rendering, PGO/LTO, and vendor-specific CPU/GPU tuning.

This ordering lets AI-generated games optimize content and recipe choices now while engine-level optimizations remain gated by workload-specific evidence.

## Context

- Parent selection gate: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`.
- Research record: `docs/specs/2026-06-01-engine-performance-optimization-foundation-v1.md`.
- Completed prerequisite: `game.agent.json.performanceBudgets` and `engine/agent/manifest.json.aiOperableProductionLoop.performanceBudgetEvidenceLoops`.
- Existing measurement substrate: `engine/core/include/mirakana/core/diagnostics.hpp`, `engine/core/src/diagnostics.cpp`, `tests/unit/core_tests.cpp`, and `tools/validate-installed-desktop-runtime.ps1`.
- First package lane candidate: `games/sample_2d_desktop_runtime_package/game.agent.json` and its installed runtime profile-resume smoke.

## Constraints

- Keep `unsupportedProductionGaps = []`; this is a developer-owned optimization foundation wave, not a reopened 1.0 blocker.
- Do not implement a broad job system, global allocator, CPU affinity policy, NUMA placement policy, renderer/RHI async-overlap claim, CUDA/HIP path, or vendor-specific optimization in this plan's first slice.
- Do not claim NVIDIA, AMD, Intel, Vulkan, Metal, D3D12, or cross-backend parity from a single host/backend trace.
- Do not expose native handles, backend pointers, allocator objects, queue/fence handles, or profiler SDK objects to gameplay APIs.
- Keep validation fail-closed: missing counters, trace artifacts, host tools, or recipe ids produce blocked, host-gated, or unsupported evidence, not a ready optimization claim.

## Phase 0 - Selection And Closeout Sync

**Files:**
- Modify: `docs/superpowers/plans/2026-06-01-ai-operable-performance-budget-and-evidence-v1.md`
- Create: `docs/superpowers/plans/2026-06-01-performance-baseline-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/01-one-dot-zero-readiness-ledger.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Generate: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration-010-agent-baseline.ps1`
- Modify: `tools/check-ai-integration-020-engine-manifest.ps1`
- Modify: `tools/check-ai-integration-030-runtime-rendering.ps1`
- Modify: `tools/check-json-contracts-030-tooling-contracts.ps1`
- Modify: `tools/check-json-contracts-040-agent-surfaces.ps1`

- [x] Mark `ai-operable-performance-budget-and-evidence-v1` completed with PR #361 closeout evidence.
- [x] Select this plan as `currentActivePlan` and `recommendedNextPlan.id = performance-baseline-v1`.
- [x] Keep `unsupportedProductionGaps = []` and update registry, roadmap, master plan, ledger, manifest, and static checks.
- [x] Run focused static validation and full repository validation for the closeout/selection slice.
- [ ] Publish a PR with validation evidence.

## Phase 1 - Diagnostics Percentile Budget Helpers

**Files:**
- Modify: `engine/core/include/mirakana/core/diagnostics.hpp`
- Modify: `engine/core/src/diagnostics.cpp`
- Test: `tests/unit/core_tests.cpp`
- Modify when public behavior lands: `docs/current-capabilities.md`

- [ ] Add a small diagnostics budget summary type over existing `CounterSample` and `ProfileSample` rows that reports count, min, average, p95, p99, max, missing-sample diagnostics, and threshold status.
- [ ] Add unit tests for sorted, unsorted, one-sample, empty, non-finite, and threshold-fail cases.
- [ ] Keep the helper data-only and dependency-free inside `MK_core`; do not add platform timers, profiler SDKs, threads, file IO, or renderer/RHI dependencies.

## Phase 2 - Selected 2D Package Baseline Smoke

**Files:**
- Modify: `tools/validate-installed-desktop-runtime.ps1`
- Modify: `games/sample_2d_desktop_runtime_package/game.agent.json`
- Modify only if existing counters are insufficient: `games/sample_2d_desktop_runtime_package/main.cpp`
- Test: focused installed-runtime validation recipe selected by the modified manifest

- [ ] Add a selected `installed-2d-performance-baseline-smoke` recipe or extend the existing profile-resume smoke with a fail-closed performance-baseline requirement.
- [ ] Require deterministic run settings, warm-up policy, fixed sample count or fixed frame count, and explicit counter names before computing p95/p99.
- [ ] Fail only on missing evidence or threshold violation for the selected lane; do not infer broad generated-game, GPU, vendor, or backend readiness.

## Phase 3 - Trace And Artifact Evidence Rows

**Files:**
- Modify: `docs/ai-game-development.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `docs/current-capabilities.md`
- Modify if schema needs explicit artifact fields: `schemas/game-agent.schema.json`
- Modify if schema changes: `tools/check-json-contracts-020-game-contracts.ps1`

- [ ] Connect the selected baseline recipe to Trace Event JSON artifact references and package-visible counter names.
- [ ] Document how an AI agent should read budgets, choose lower-cost content, rerun the selected recipe, and file an engine capability handoff when required evidence is absent.
- [ ] Preserve unsupported claims for cross-vendor parity, cross-backend parity, allocator enforcement, all-core CPU use, CUDA/HIP, GPU-driven rendering, and Metal readiness.

## Phase 4 - Closeout And Next Wave Selection

**Files:**
- Modify: this plan
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Generate: `engine/agent/manifest.json`

- [ ] Record final validation evidence and hosted PR evidence.
- [ ] Select the next optimization wave, likely `memory-lifetime-taxonomy-v1`, unless the baseline evidence shows a more urgent measured bottleneck.
- [ ] Keep completed evidence concise and avoid copying long benchmark output into durable docs.

## Validation Evidence

Closeout/selection slice validation on 2026-06-01:

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`
- `git diff --check`

All listed commands passed in the candidate worktree. `tools/validate.ps1` reported expected diagnostic-only Windows host blockers for Metal/Apple lanes while returning `validate: ok`.

Future C++/runtime/package slices must also run the smallest focused unit/package checks first and close with `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` when public/runtime contracts change.

## Done When

- A selected package lane can produce deterministic frame/counter/profile samples and p95/p99 budget results.
- The selected budget row links to a validation recipe, trace or counter artifact, host/backend scope, and unsupported claims.
- Missing host evidence, missing trace artifacts, and over-budget results fail closed.
- AI agents can answer which performance budget was optimized, which recipe proved it, and which CPU/GPU/memory optimization claims remain unsupported.
