# 2026-06-02 Memory Diagnostics v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add the first AI-operable memory diagnostics layer so future allocator, pool, arena, GPU residency, and async-upload optimization work starts from class-specific counters and fail-closed evidence.

**Architecture:** Build on Memory Lifetime Taxonomy v1 and the existing dependency-free `MK_core` diagnostics surface. Memory diagnostics must report bytes/counts by lifetime class, high-water marks, budget pressure, stale-generation and use-after-safe-point diagnostics, and package-visible evidence rows before any allocator replacement, all-core scheduling, NUMA/affinity, SIMD dispatch, GPU residency, CUDA/HIP/SYCL, or vendor-specific tuning claim is made.

**Tech Stack:** C++23 `MK_core` diagnostics, existing package performance-budget/evidence rows, existing RHI read-only memory diagnostics and GPU memory policy rows, PowerShell 7 validation wrappers, composed `engine/agent/manifest.json`, and static checks under `tools/check-ai-integration*.ps1` / `tools/check-json-contracts*.ps1`.

---

**Plan ID:** `memory-diagnostics-v1`

**Status:** Active.

Selected on 2026-06-02 after [Memory Lifetime Taxonomy v1](2026-06-01-memory-lifetime-taxonomy-v1.md) completed the ownership, pointer/view, handle, and memory-class taxonomy needed before measurable memory optimization work.

## Ordering Decision

Do memory diagnostics before allocator or scheduler implementation:

- Required first: lifetime-class counter names, byte/count accumulation, high-water marks, budget pressure state, leak/stale-generation/use-after-safe-point diagnostic codes, and package-visible evidence rows.
- Safe early implementation: dependency-free data summaries in `MK_core`, selected package smoke counters, and documentation that maps existing RHI/GPU memory policy rows to narrow claims.
- Delay until diagnostics exist: global allocator replacement, pool/arena enforcement, thread-local scratch enforcement, all-core job execution, NUMA placement, SIMD dispatch, automatic/LRU GPU residency, CUDA/HIP/SYCL adapters, and broad cross-vendor/backend memory performance claims.

This keeps optimization work measurable without exposing allocator internals, backend-native memory handles, queue/fence handles, or profiler SDK objects through gameplay APIs.

## Context

- Parent selection gate: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`.
- Completed prerequisites: [Performance Baseline v1](2026-06-01-performance-baseline-v1.md) and [Memory Lifetime Taxonomy v1](2026-06-01-memory-lifetime-taxonomy-v1.md).
- Research record: `docs/specs/2026-06-01-engine-performance-optimization-foundation-v1.md`.
- Existing diagnostics substrate: `engine/core/include/mirakana/core/diagnostics.hpp`, `engine/core/src/diagnostics.cpp`, package `performanceBudgets`, `performanceBudgetEvidenceLoops`, RHI read-only memory diagnostics, and `gpu_memory_policy_*` package evidence.

## Constraints

- Keep `unsupportedProductionGaps = []`; this is a developer-owned optimization foundation wave, not a reopened 1.0 blocker.
- Do not replace allocators, enforce pools/arenas, add a job scheduler, set CPU affinity, add NUMA placement, add SIMD dispatch, change GPU residency policy, or add CUDA/HIP/SYCL runtime paths in this plan until class diagnostics exist and pass focused validation.
- Do not expose native allocator handles, backend memory objects, OS heap handles, GPU heap handles, queues, fences, profiler SDK objects, or pointer-owned gameplay APIs.
- Keep all broad CPU/GPU/memory optimization, cross-vendor parity, cross-backend parity, all-core CPU usage, and vendor-specific tuning claims fail-closed until a selected recipe has focused evidence for that exact claim.

## Phase 0 - Taxonomy Closeout And Selection Sync

**Files:**
- Modify: `docs/superpowers/plans/2026-06-01-memory-lifetime-taxonomy-v1.md`
- Create: this plan
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/01-one-dot-zero-readiness-ledger.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Generate: `engine/agent/manifest.json`
- Modify: relevant `tools/check-ai-integration*.ps1` and `tools/check-json-contracts*.ps1` files for `memory-diagnostics-v1` durable literals

- [x] Mark Memory Lifetime Taxonomy v1 completed with Phase 1, Phase 2, Phase 3, local validation, hosted PR, and merge evidence.
- [x] Select this plan as `currentActivePlan` and `recommendedNextPlan.id = memory-diagnostics-v1`.
- [x] Keep `unsupportedProductionGaps = []` and update registry, roadmap, current capabilities, master plan, ledger, manifest, and static checks.
- [x] Run focused static validation and publish a PR with hosted check evidence.

Phase 0 closeout on 2026-06-02 selected this plan as the active memory diagnostics wave after Memory Lifetime Taxonomy v1 completed. Registry, roadmap, current capabilities, production master plan, readiness ledger, manifest fragments, composed manifest, and static-check needles now agree on `memory-diagnostics-v1` while preserving historical manifest literals for renderer/RHI, editor UI, cross-platform editor gates, and VFX/profiling evidence.

## Phase 1 - Core Memory Diagnostics Data Model

**Files:**
- Modify: `engine/core/include/mirakana/core/diagnostics.hpp`
- Modify: `engine/core/src/diagnostics.cpp`
- Test: `tests/unit/core_tests.cpp`
- Modify when public behavior lands: `docs/current-capabilities.md`

- [x] Add dependency-free data types for lifetime class, memory counter rows, high-water summaries, budget pressure summaries, and diagnostic codes.
- [x] Add unit tests for empty input, class aggregation, high-water updates, budget pressure, stale-generation diagnostics, use-after-safe-point diagnostics, invalid byte/count rows, and threshold failures.
- [x] Keep the model data-only; do not add global allocators, hooks, threads, OS heap calls, GPU APIs, file IO, or profiler SDK integrations.

Phase 1 closeout on 2026-06-02 adds `MemoryLifetimeClass`, `MemoryCounterRow`, `MemoryDiagnosticsOptions`, `MemoryClassDiagnosticsSummary`, `MemoryDiagnosticsSummary`, `MemoryBudgetPressure`, `MemoryDiagnosticsCode`, `MemoryDiagnosticsStatus`, `summarize_memory_diagnostics`, and stable label helpers to `MK_core`. The summary aggregates bytes, allocation counts, high-water bytes, budget pressure, invalid counter rows, stale-generation diagnostics, and use-after-safe-point diagnostics by lifetime class. It is deliberately data-only: package-visible `memory_diagnostics_*` counters, allocator hooks/enforcement, pool/arena policy, GPU residency policy, all-core scheduling, NUMA/affinity, SIMD dispatch, CUDA/HIP/SYCL paths, cross-vendor parity, cross-backend parity, and broad memory optimization remain unsupported until later focused evidence.

## Phase 2 - Package And RHI Evidence Mapping

**Files:**
- Modify: selected sample package `game.agent.json` rows only when evidence is added
- Modify: selected package smoke source only when counters are actually emitted
- Modify: `docs/ai-game-development.md`
- Modify: `docs/current-capabilities.md`
- Modify when needed: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Generate when manifest changes: `engine/agent/manifest.json`
- Modify when checks need new needles: relevant `tools/check-ai-integration*.ps1` and `tools/check-json-contracts*.ps1`

- [ ] Define package-visible `memory_diagnostics_*` counter names for the selected lane before wiring runtime output.
- [ ] Map existing RHI read-only memory diagnostics, upload/staging byte counters, transient alias allocation counters, and `gpu_memory_policy_*` rows to narrow memory-evidence claims.
- [ ] Preserve unsupported claims for allocator enforcement, automatic/LRU GPU residency, cross-vendor/backend parity, CUDA/HIP/SYCL, all-core scheduling, NUMA/affinity, and broad memory optimization.

## Phase 3 - Closeout And Next Wave Selection

**Files:**
- Modify: this plan
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/01-one-dot-zero-readiness-ledger.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Generate: `engine/agent/manifest.json`
- Modify: relevant static checks only when the selected next wave adds durable literals

- [ ] Record final validation evidence and hosted PR evidence.
- [ ] Select the next measured implementation wave, likely transient-frame allocation counters, worker scratch/job scheduling evidence, or renderer/RHI GPU residency diagnostics depending on Phase 1/2 results.
- [ ] Keep completed evidence concise and avoid copying long command output into durable docs.

## Validation Evidence

Close each behavior or contract slice with the smallest relevant focused checks first. A manifest/docs/static-check slice should normally include:

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`
- `git diff --check`

Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` at the slice-closing gate when C++/runtime/build/packaging/public contracts change, or when narrower checks cannot prove the changed surface.

Phase 0 closeout validation on 2026-06-02:

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`
- `git diff --check`
- Full `tools/validate.ps1` was not run for this Phase 0 closeout because the change is limited to docs, manifest pointers, and static contract needles; targeted manifest/agent/static checks cover the changed contracts.
- Hosted PR evidence is recorded on the task-owned Phase 0 PR.

Phase 1 focused validation on 2026-06-02:

- TDD red check: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests` failed before implementation because the new memory diagnostics API did not exist.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_core_tests`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -ReuseExistingFileApiReply -Files engine/core/src/diagnostics.cpp,tests/unit/core_tests.cpp`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`
- `git diff --check`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with `validate: ok`; Windows host Metal/Apple checks remained diagnostic-only or host-gated as expected, and 85 CTest tests passed.

## Done When

- The engine has AI-readable memory diagnostics by lifetime class with bytes/counts, high-water marks, budget pressure, and stale-generation/use-after-safe-point diagnostics.
- Selected package or runtime evidence can identify which memory class changed and which recipe proved the claim.
- Existing RHI/GPU memory evidence is mapped to narrow ready claims without implying allocator enforcement or automatic residency.
- Allocator, pool, arena, all-core scheduling, NUMA, SIMD, GPU residency, CUDA/HIP/SYCL, and vendor-specific optimization claims remain unsupported until focused implementation and validation evidence exists.
