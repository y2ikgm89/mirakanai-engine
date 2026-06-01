# 2026-06-02 Frame/Thread Scratch v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add AI-operable frame temporary and worker scratch memory evidence so future arena enforcement, allocator replacement, job scheduling, and CPU/GPU memory optimization work starts from explicit ownership APIs and high-water diagnostics.

**Architecture:** Build on Memory Lifetime Taxonomy v1 and Memory Diagnostics v1. This plan should introduce first-party frame scratch and per-worker scratch ownership surfaces, high-water/reset/cross-thread diagnostic evidence, and selected package-visible counters without replacing global allocators, changing all-core scheduling, exposing native handles, or claiming broad CPU/GPU/memory optimization.

**Tech Stack:** C++23 `MK_core` memory diagnostics, existing frame/RHI transient allocation and aliasing statistics where useful, selected package evidence, PowerShell 7 validation wrappers, composed `engine/agent/manifest.json`, and static checks under `tools/check-ai-integration*.ps1` / `tools/check-json-contracts*.ps1`.

---

**Plan ID:** `frame-thread-scratch-v1`

**Status:** Active.

Selected on 2026-06-02 after [Memory Diagnostics v1](2026-06-02-memory-diagnostics-v1.md) completed class-specific memory diagnostics and selected package-visible memory evidence.

## Ordering Decision

Do frame/thread scratch evidence before allocator replacement, broad job scheduling, NUMA/affinity tuning, or GPU residency policy:

- Memory Diagnostics v1 covers class-specific counters and package evidence, but `frame_temporary` and `worker_scratch` still need first-party ownership and high-water evidence.
- First-party frame arenas and per-worker scratch arenas can be introduced behind explicit value/ownership APIs before global allocator replacement.
- Existing RHI/frame transient statistics can feed this wave as measurement evidence, but automatic/LRU GPU residency policy remains a separate future selection.
- Worker scratch evidence should precede broad all-core scheduling so cross-thread frees, reset boundaries, false-sharing diagnostics, and safe-point lifetime rules are visible before scheduler tuning.

## Official References

- Microsoft Direct3D 12 resource barriers and aliasing barriers: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12>
- Microsoft Direct3D 12 memory management strategies and placed/reserved resources: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/memory-management-strategies>
- Vulkan memory allocation and aliasing rules: <https://docs.vulkan.org/spec/latest/chapters/memory.html>
- Vulkan image alias flag reference: <https://docs.vulkan.org/refpages/latest/refpages/source/VkImageCreateFlagBits.html>

## Context

- Parent selection gate: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`.
- Completed prerequisites: [Performance Baseline v1](2026-06-01-performance-baseline-v1.md), [Memory Lifetime Taxonomy v1](2026-06-01-memory-lifetime-taxonomy-v1.md), and [Memory Diagnostics v1](2026-06-02-memory-diagnostics-v1.md).
- Research record: `docs/specs/2026-06-01-engine-performance-optimization-foundation-v1.md`.
- Existing evidence substrate: `MemoryCounterRow`, `summarize_memory_diagnostics`, package `memory_diagnostics_*` counters, RHI read-only memory diagnostics, frame graph transient alias allocation counters, upload/staging counters, and selected `gpu_memory_policy_*` package evidence.

## Constraints

- Keep `unsupportedProductionGaps = []`; this is a developer-owned optimization foundation wave, not a reopened 1.0 blocker.
- Do not replace global allocators, change game object ownership, add broad job scheduling, set CPU affinity, add NUMA placement, add SIMD dispatch, change GPU residency policy, or add CUDA/HIP/SYCL runtime paths in this plan.
- Do not expose native allocator handles, backend memory objects, OS heap handles, GPU heap handles, queues, fences, profiler SDK objects, or pointer-owned gameplay APIs.
- Keep cross-vendor parity, cross-backend parity, broad all-core CPU usage, broad GPU memory residency, and broad CPU/GPU/memory optimization claims fail-closed until focused implementation and validation evidence exists.

## Phase 0 - Selection Sync

**Files:**
- Modify: [Memory Diagnostics v1](2026-06-02-memory-diagnostics-v1.md)
- Create: this plan
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/specs/2026-06-01-engine-performance-optimization-foundation-v1.md`
- Modify: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/01-one-dot-zero-readiness-ledger.md`
- Modify when useful: `docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Generate: `engine/agent/manifest.json`
- Modify: relevant `tools/check-ai-integration*.ps1` / `tools/check-json-contracts*.ps1` files for `frame-thread-scratch-v1` durable literals

- [x] Mark Memory Diagnostics v1 completed with Phase 1, Phase 2, Phase 3, local validation, hosted PR, and merge evidence.
- [x] Select this plan as `currentActivePlan` and `recommendedNextPlan.id = frame-thread-scratch-v1`.
- [x] Keep `unsupportedProductionGaps = []` and update registry, roadmap, current capabilities, master plan, ledger, manifest, and static checks.
- [x] Record official D3D12/Vulkan aliasing and memory-management references for the implementation phases.

## Phase 1 - Core Frame/Worker Scratch Evidence Model

**Files:**
- Modify: `engine/core/include/mirakana/core/diagnostics.hpp` only if a reusable data-summary extension is needed
- Modify: `engine/core/src/diagnostics.cpp` only if a reusable data-summary extension is needed
- Test: `tests/unit/core_tests.cpp` when `MK_core` behavior changes

- [x] Define or reuse data-only rows that can summarize frame temporary and worker scratch bytes, allocations, high-water marks, reset counts, cross-thread frees, stale safe-point use, and false-sharing diagnostics by lifetime class.
- [x] Keep the model host-independent and dependency-free.
- [x] Add focused tests only for durable external behavior, not for incidental implementation ordering.

Phase 1 implementation on 2026-06-02 extends the dependency-free `MK_core` `MemoryCounterRow`, `MemoryClassDiagnosticsSummary`, and `MemoryDiagnosticsSummary` rows with reuse counts, reset counts, cross-thread-free counts, use-after-safe-point counts, and false-sharing counts. `summarize_memory_diagnostics` now aggregates those values by lifetime class, treats zero-allocation reuse as valid evidence only for `frame_temporary` and `worker_scratch`, deduplicates legacy boolean and counted use-after-safe-point reports, and reports `cross_thread_free` / `false_sharing` diagnostics without installing allocators, enforcing arenas, or changing scheduler behavior.

## Phase 2 - First-Party Scratch Ownership APIs

**Files:**
- Modify: selected `engine/core/` or runtime files only after Phase 1 evidence names are stable
- Test: focused core/runtime tests matching touched code

- [ ] Add explicit first-party frame scratch and per-worker scratch ownership APIs with deterministic reset/safe-point boundaries.
- [ ] Keep raw pointers non-owning and expose scratch leases through bounded value/view types rather than pointer-owned gameplay APIs.
- [ ] Preserve global allocator behavior and avoid broad scheduler or CPU-affinity changes.

## Phase 3 - RHI/Package Evidence Mapping

**Files:**
- Modify: selected `engine/rhi/`, `engine/renderer/`, runtime-host, or sample package files only where counters are emitted
- Modify: selected sample `game.agent.json` rows only when evidence is added
- Modify: `tools/validate-installed-desktop-runtime.ps1` only when installed package validation gains new required fields
- Modify: docs/manifest/static checks for new durable counter names

- [ ] Map existing transient texture heap/placed allocation counters and aliasing-barrier evidence into the frame/thread scratch diagnostic story where appropriate.
- [ ] Add selected package-visible counter names before wiring validator expectations.
- [ ] Keep zero or host-dependent values honest; do not require positive transient allocation counts unless the selected path actually produces them.

## Phase 4 - Closeout And Next Wave Selection

**Files:**
- Modify: this plan
- Modify: registry/roadmap/current-capabilities/master-plan/ledger/manifest/static checks as needed

- [ ] Record final validation and hosted PR evidence.
- [ ] Select the next measured wave, likely job scheduling evidence, GPU residency diagnostics, or allocator enforcement depending on Phase 1-3 results.
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

Phase 0 selection validation on 2026-06-02:

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`
- `git diff --check`
- Full `tools/validate.ps1` was not run for this Phase 0 selection because the change is limited to docs, manifest pointers, and static contract needles; targeted manifest/agent/static checks cover the changed contracts.
- Hosted PR evidence is recorded on the task-owned Phase 0 selection PR.

Phase 1 focused validation on 2026-06-02:

- TDD red check: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests` failed before implementation because `MemoryCounterRow` did not yet expose `reuse_count`, `reset_count`, `cross_thread_free_count`, `use_after_safe_point_count`, `false_sharing_count`, and the summary rows did not expose matching totals or diagnostic codes.
- Review red check: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev -R MK_core_tests --output-on-failure` failed after adding boundary tests for non-scratch zero-allocation reuse and legacy/count use-after-safe-point double counting.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev -R MK_core_tests --output-on-failure`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files "engine/core/src/diagnostics.cpp,tests/unit/core_tests.cpp"`
- `git diff --check`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`
- Hosted PR evidence is recorded on the task-owned Phase 1 PR.

## Done When

- The engine reports selected frame temporary and worker scratch allocation/reuse/reset/high-water counters in AI-readable diagnostic rows.
- First-party frame and per-worker scratch ownership APIs are explicit, bounded, and reset at reviewed safe points.
- Selected package evidence can identify frame temporary and transient GPU allocation pressure without guessing backend internals.
- Allocator replacement, broad job scheduling, all-core CPU usage, NUMA, SIMD, GPU residency policy, CUDA/HIP/SYCL, cross-vendor parity, cross-backend parity, and vendor-specific optimization claims remain unsupported until focused implementation and validation evidence exists.
