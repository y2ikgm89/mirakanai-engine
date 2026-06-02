# 2026-06-02 Job Scheduling Evidence v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add AI-operable CPU job scheduling evidence so future all-core scheduling, affinity, NUMA, SIMD, and vendor-specific CPU optimization work starts from deterministic topology, queue, scratch, and budget data instead of broad utilization claims.

**Architecture:** Build on Performance Baseline v1, Memory Lifetime Taxonomy v1, Memory Diagnostics v1, and Frame/Thread Scratch v1. This plan should introduce first-party worker topology rows, bounded job queue evidence, deterministic per-worker scratch usage rows, queue/safe-point diagnostics, and package-visible scheduler budget evidence where selected. It must not hard-pin threads, claim all-core CPU scheduling, add process-wide allocator replacement, or add vendor-specific SIMD/runtime dispatch until focused host traces justify those changes.

**Tech Stack:** C++23 `MK_core` / selected runtime diagnostic rows, existing `ScratchArena` ownership evidence, package-visible counters where useful, PowerShell 7 validation wrappers, composed `engine/agent/manifest.json`, and static checks under `tools/check-ai-integration*.ps1` / `tools/check-json-contracts*.ps1`.

---

**Plan ID:** `job-scheduling-evidence-v1`

**Status:** Active.

Selected on 2026-06-02 after [Frame/Thread Scratch v1](2026-06-02-frame-thread-scratch-v1.md) completed frame temporary and worker scratch ownership APIs plus selected transient GPU aliasing package evidence.

## Ordering Decision

Do job scheduling evidence before broad all-core CPU scheduling, affinity pinning, NUMA placement, SIMD dispatch, or vendor-specific tuning:

- Frame/Thread Scratch v1 made per-frame and per-worker temporary memory explicit enough for scheduler evidence to charge worker scratch use, reset counts, and misuse diagnostics.
- A scheduler without topology, queue, steal, wait, and merge evidence cannot prove that it improves frame time or uses cores safely.
- CPU utilization is not the goal by itself; stable p95/p99 frame time, bounded queue latency, deterministic merge behavior, and no driver/audio/platform starvation matter more than occupying every hardware thread.
- Intel hybrid P-core/E-core systems, AMD CCX/CCD/NUMA locality, Windows processor groups, SMT, and driver/helper threads need measured policies rather than a single default "use all cores" switch.
- GPU optimization and async queue work depend on CPU submission/scheduling evidence; do not tune GPU residency or async overlap by hiding CPU-side job bottlenecks.

## Official References

- Intel 64 and IA-32 Architectures Optimization Reference Manual: <https://www.intel.com/content/www/us/en/developer/articles/technical/intel64-and-ia32-architectures-optimization.html>
- Intel VTune Profiler User Guide and Top-down Microarchitecture Analysis: <https://www.intel.com/content/www/us/en/docs/vtune-profiler/user-guide/2024-2/overview.html>, <https://www.intel.com/content/www/us/en/docs/vtune-profiler/cookbook/2024-0/top-down-microarchitecture-analysis-method.html>
- Intel Performance Hybrid Architecture material: <https://www.intel.com/content/www/us/en/developer/articles/technical/hybrid-architecture.html>
- AMD uProf User Guide and AMD EPYC Software Optimization Guide: <https://docs.amd.com/r/en-US/57368-uProf-user-guide/uProf-User-Guide>, <https://docs.amd.com/v/u/en-US/56305>
- Microsoft Processor Groups and NUMA support: <https://learn.microsoft.com/en-us/windows/win32/procthread/processor-groups>, <https://learn.microsoft.com/en-us/windows/win32/procthread/numa-support>
- oneTBB scheduler and allocator guidance: <https://uxlfoundation.github.io/oneTBB/main/tbb_userguide/How_Task_Scheduler_Works.html>, <https://uxlfoundation.github.io/oneTBB/main/tbb_userguide/Memory_Allocation.html>
- Robert D. Blumofe and Charles E. Leiserson, "Scheduling Multithreaded Computations by Work Stealing": <https://epubs.siam.org/doi/10.1137/S0097539793259471>
- Gene Amdahl, "Validity of the Single Processor Approach to Achieving Large Scale Computing Capabilities": <https://www.cs.cmu.edu/~18742/papers/Amdahl1967.pdf>
- John L. Gustafson, "Reevaluating Amdahl's Law": <https://cacm.acm.org/research/reevaluating-amdahls-law/>

## Context

- Parent selection gate: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`.
- Completed prerequisites: [Performance Baseline v1](2026-06-01-performance-baseline-v1.md), [Memory Lifetime Taxonomy v1](2026-06-01-memory-lifetime-taxonomy-v1.md), [Memory Diagnostics v1](2026-06-02-memory-diagnostics-v1.md), and [Frame/Thread Scratch v1](2026-06-02-frame-thread-scratch-v1.md).
- Research record: `docs/specs/2026-06-01-engine-performance-optimization-foundation-v1.md`.
- Existing evidence substrate: `ScratchArena`, `ScratchLease`, `MemoryCounterRow`, `MemoryDiagnosticsSummary`, selected package performance budgets, package memory diagnostics, trace JSON references, and package-visible framegraph aliasing evidence.

## Constraints

- Keep `unsupportedProductionGaps = []`; this is a developer-owned optimization foundation wave, not a reopened 1.0 blocker.
- Do not claim all-core CPU scheduling, CPU saturation, affinity policy, NUMA placement, hybrid P-core/E-core policy, SMT policy, SIMD dispatch, GPU async overlap, CUDA/HIP/SYCL execution, or cross-vendor CPU/GPU parity in this plan until focused phases add evidence.
- Do not expose OS thread handles, native affinity masks, processor-group handles, profiler SDK objects, RHI queues/fences, or pointer-owned gameplay APIs.
- Keep raw pointers non-owning and keep job data movement through value rows, handles, spans, and explicit ownership transfer.
- Treat bitset, bitboard, and bit exhaustive-search algorithms as bounded algorithm-specific tools. They can be useful for masks, archetype/query filters, board/tile pattern lookup, small subset DP, and offline tooling, but they must not become a general scheduler strategy or unbounded runtime search path.

## Phase 0 - Selection Sync

**Files:**
- Modify: [Frame/Thread Scratch v1](2026-06-02-frame-thread-scratch-v1.md)
- Create: this plan
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/specs/2026-06-01-engine-performance-optimization-foundation-v1.md`
- Modify: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify when useful: `docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Generate: `engine/agent/manifest.json`
- Modify: relevant `tools/check-ai-integration*.ps1` / `tools/check-json-contracts*.ps1` files for `job-scheduling-evidence-v1` durable literals

- [x] Mark Frame/Thread Scratch v1 completed with Phase 1, Phase 2, Phase 3, local validation, hosted PR, and merge evidence.
- [x] Select this plan as `currentActivePlan` and `recommendedNextPlan.id = job-scheduling-evidence-v1`.
- [x] Keep `unsupportedProductionGaps = []` and update registry, roadmap, current capabilities, master plan, backlog, manifest, and static checks.
- [x] Record official Intel, AMD, Microsoft, oneTBB, and scheduling-theory references for implementation phases.

## Phase 1 - Worker Topology And Scheduler Evidence Rows

**Files:**
- Modify: selected `engine/core/` or runtime diagnostics files after evidence names are stable
- Test: focused core/runtime tests matching touched code

- [x] Define worker topology rows for logical processor count, worker count, queue count, processor-group awareness, NUMA-awareness status, and host-gated topology diagnostics through `JobWorkerTopologyRow`.
- [x] Define job scheduling diagnostic rows for submitted/completed jobs, queue capacity, queue depth high-water, queue overflow, steal attempts/successes, waits, blocked dependencies, dependency cycles, deterministic/non-deterministic merge rows, scratch misuse, undersized batches, and oversized batches through `JobQueueCounterRow` plus `JobSchedulingDiagnosticsSummary`.
- [x] Keep evidence host-independent by default; platform adapters may supply topology facts later, but public engine APIs must not expose OS handles, affinity masks, NUMA placement controls, thread pools, or native scheduler objects.

## Phase 2 - Bounded Scheduler/Queue Execution Evidence

**Files:**
- Modify: selected `engine/core/` or runtime scheduling files only after Phase 1 row names are stable
- Test: focused scheduler/core/runtime tests matching touched code

- [x] Add a deterministic bounded queue or scheduler evidence path with explicit job dependencies, worker-local outputs, and deterministic merge behavior.
- [x] Charge per-worker scratch use through existing scratch evidence where jobs request temporary memory.
- [x] Detect oversized jobs, too-small job batches, queue overflow, dependency cycles, cross-thread scratch misuse, and non-deterministic merge attempts.
- [x] Preserve OS scheduling defaults; no affinity, NUMA, or SIMD tuning in this phase unless a later scoped phase adds evidence first.

## Phase 3 - Package And AI Evidence Mapping

**Files:**
- Modify: selected sample package files only where counters are emitted
- Modify: selected `game.agent.json` rows only when evidence is added
- Modify: selected validators only when installed package validation gains new required fields
- Modify: docs/manifest/static checks for new durable counter names

- [ ] Add selected package-visible scheduler counters when a sample path can honestly exercise the scheduler evidence.
- [ ] Add AI-facing guidance so generated games can request reviewed job scheduling evidence without editing engine internals.
- [ ] Keep broad all-core, affinity, NUMA, SIMD, GPU async overlap, and vendor-specific tuning claims unsupported until measured follow-up plans land.

## Validation Evidence

Close each behavior or contract slice with the smallest relevant focused checks first. A manifest/docs/static-check slice should normally include:

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`
- `git diff --check`

Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` at the slice-closing gate when C++/runtime/build/packaging/public contracts change, or when narrower checks cannot prove the changed surface.

Phase 0 selection validation on 2026-06-02:

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`
- `git diff --check`
- Full `tools/validate.ps1` was not run for this Phase 0 selection because the change is limited to docs, manifest pointers, and static contract needles; targeted manifest/agent/static checks cover the changed contracts.

Phase 1 focused validation on 2026-06-02:

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev`
- TDD red: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests` failed on missing `JobWorkerTopologyRow`, `JobQueueCounterRow`, `JobSchedulingDiagnosticsStatus`, `JobSchedulingDiagnosticsCode`, `summarize_job_scheduling_diagnostics`, `job_scheduling_diagnostics_code_label`, and `job_scheduling_diagnostics_status_label` before implementation.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_core_tests`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files 'engine/core/src/diagnostics.cpp,tests/unit/core_tests.cpp'`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`

Phase 2 focused validation on 2026-06-02:

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev`
- TDD red: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests` failed on missing `JobSchedulingWorkItemRow`, `JobSchedulingExecutionOptions`, and `build_job_scheduling_execution_evidence` before implementation.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_core_tests`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files 'engine/core/src/diagnostics.cpp,tests/unit/core_tests.cpp'`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`

## Done When

- The engine exposes selected worker topology, bounded scheduler, queue, scratch, wait, steal, merge, and budget evidence in AI-readable diagnostic rows.
- Selected package evidence can prove scheduler participation without guessing thread state or relying on CPU utilization alone.
- AI-generated games can identify when scheduler evidence exists and when a requested engine capability needs a developer-owned handoff.
- All-core CPU scheduling, affinity, NUMA placement, hybrid CPU policy, SMT policy, SIMD dispatch, CUDA/HIP/SYCL, GPU async overlap, cross-vendor parity, cross-backend parity, and broad CPU/GPU/memory optimization claims remain unsupported until focused implementation and validation evidence exists.
