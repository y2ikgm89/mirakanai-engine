# 2026-06-01 Memory Lifetime Taxonomy v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Define the AI-operable memory, lifetime, pointer, and ownership taxonomy needed before allocator replacement, all-core job scheduling, NUMA/affinity tuning, GPU memory residency expansion, or vendor-specific CPU/GPU optimization work.

**Architecture:** Keep the taxonomy aligned with existing C++23 rules: RAII and value types by default, `std::unique_ptr` for unique ownership, raw pointers and references as non-owning observation only, `std::span` / string views for bounded non-owning ranges, first-party opaque handles for backend objects, and explicit diagnostics rows for memory budgets and unsupported optimization claims.

**Tech Stack:** C++23 guidance, existing `MK_core` diagnostics and budget helpers, existing package performance budget rows, composed `engine/agent/manifest.json`, PowerShell 7 validation wrappers, and static checks under `tools/check-ai-integration*.ps1` / `tools/check-json-contracts*.ps1`.

---

**Plan ID:** `memory-lifetime-taxonomy-v1`

**Status:** Active.

Selected on 2026-06-01 after [Performance Baseline v1](2026-06-01-performance-baseline-v1.md) completed the first measurement/evidence lane for selected package frame budgets, p95/p99 counters, and Trace Event JSON artifact references.

## Ordering Decision

Do the memory/lifetime taxonomy before broad implementation:

- Required first: ownership vocabulary, allowed pointer/view semantics, object lifetime boundaries, allocator ownership boundaries, thread/job handoff rules, backend handle opacity, and AI-visible unsupported-claim wording.
- Safe early reviews: current public header audit, pointer/reference member audit, container/view API audit, transient-frame allocation inventory, GPU resource lifetime rows, and profiler/counter naming requirements.
- Delay until this taxonomy exists: global allocator replacement, arena/pool enforcement, job graph execution, cross-core affinity policy, NUMA placement, SIMD dispatch tables, renderer/RHI async overlap, GPU residency manager expansion, CUDA/HIP/SYCL adapters, and vendor-specific tuning.

This preserves optimization headroom while keeping AI-generated game code from depending on dangling views, hidden ownership transfer, backend-native handles, or premature vendor-specific behavior.

## Reference Anchors

Use these official or primary sources as source-of-truth anchors during implementation:

- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines) for RAII, `std::unique_ptr` ownership, raw pointer non-ownership, and span-style range interfaces.
- [Intel 64 and IA-32 Architectures Optimization Reference](https://www.intel.com/content/www/us/en/developer/articles/technical/intel64-and-ia32-architectures-optimization.html) for Intel CPU optimization references.
- [AMD Processor Resources](https://www.amd.com/en/developer/browse-by-product-type/processor-resources.html) for AMD EPYC/Ryzen tuning guides, AOCC, AOCL, and uProf references.
- [NVIDIA CUDA C++ Best Practices Guide](https://docs.nvidia.com/cuda/cuda-c-best-practices-guide/index.html) for CUDA memory-space, coalescing, and measure-first guidance.
- [AMD RDNA Performance Guide](https://gpuopen.com/learn/rdna-performance-guide/) for RDNA renderer/barrier guidance.
- [Intel Arc Gaming API Developer and Optimization Guide](https://www.intel.com/content/www/us/en/developer/articles/guide/arc-a-series-gaming-api-developer-optimization.html) for Intel GPU feature detection and vendor-agnostic guidance.
- [Direct3D 12 Memory Management](https://learn.microsoft.com/en-us/windows/win32/direct3d12/memory-management) for classify, budget, stream, residency, and heap/buffer suballocation guidance.
- [Vulkan Memory Allocation guide](https://docs.vulkan.org/guide/latest/memory_allocation.html) and Vulkan synchronization specification for explicit application-owned memory and synchronization responsibilities.
- [Perfetto Trace Event JSON guidance](https://perfetto.dev/docs/getting-started/other-formats) for host-attached trace artifact compatibility.

## Context

- Parent selection gate: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`.
- Completed measurement layer: [Performance Baseline v1](2026-06-01-performance-baseline-v1.md).
- Research record: `docs/specs/2026-06-01-engine-performance-optimization-foundation-v1.md`.
- Existing C++ rule baseline: `AGENTS.md`, `docs/cpp-style.md`, `docs/architecture.md`, and `docs/current-capabilities.md`.
- Existing diagnostics substrate: `engine/core/include/mirakana/core/diagnostics.hpp`, `engine/core/src/diagnostics.cpp`, package `performanceBudgets`, and `performanceBudgetEvidenceLoops`.

## Constraints

- Keep `unsupportedProductionGaps = []`; this is a developer-owned optimization foundation wave, not a reopened 1.0 blocker.
- Do not implement allocator replacement, job execution, NUMA/affinity policy, SIMD dispatch, GPU residency expansion, CUDA/HIP/SYCL adapters, or vendor-specific optimization in this taxonomy slice.
- Do not expose native handles, backend pointers, allocator internals, queue/fence handles, profiler SDK objects, or GPU memory objects through gameplay APIs.
- Do not introduce third-party dependencies or copied external code.
- Keep AI-facing claims fail-closed: missing lifetime taxonomy, missing budget counters, missing trace artifacts, or host-gated profiler evidence must remain unsupported.

## Phase 1 - Ownership And Pointer Vocabulary Audit

**Files:**
- Modify: `docs/cpp-style.md`
- Modify: `docs/architecture.md` or a focused spec if the guidance is too detailed for always-read docs
- Modify when durable AI behavior changes: `docs/ai-game-development.md`
- Modify when manifest-visible claims change: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Generate when manifest changes: `engine/agent/manifest.json`

- [x] Audit public headers for ownership-returning APIs, raw pointer/reference parameters, raw pointer/reference members, view-like parameters, and backend handle boundaries.
- [x] Define the project vocabulary for owner, observer, borrowed range, frame-local scratch, stable handle, backend-private object, and handoff/result rows.
- [x] Document when to use values, references, raw pointers, `std::unique_ptr`, `std::span`, `std::string_view`, first-party handles, and IDs.
- [x] Preserve the existing rule that raw pointers are non-owning; any ownership transfer must be explicit in type or result contract.

Phase 1 evidence on 2026-06-02: audited representative public headers for RHI resource handles and lifetime registry APIs, runtime asset/package observer lookups, runtime scene borrowed ranges, runtime/RHI execution descriptors, scene/UI/editor lookup observers, and Windows runtime-host observer descriptors. Durable guidance now lives in `docs/cpp-style.md`, `docs/architecture.md`, `docs/ai-game-development.md`, and `docs/current-capabilities.md`. Manifest fragments and static-check needles were inspected and left unchanged because they already cover `memory-lifetime-taxonomy-v1`, raw pointer non-ownership, `std::unique_ptr`, `std::span`, and allocator/job/NUMA/GPU-memory follow-up gates.

## Phase 2 - Memory Lifetime And Allocation Boundary Rows

**Files:**
- Modify: `docs/specs/2026-06-01-engine-performance-optimization-foundation-v1.md`
- Modify: `docs/current-capabilities.md`
- Modify when needed: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Generate when manifest changes: `engine/agent/manifest.json`
- Modify when checks need new needles: `tools/check-ai-integration-010-agent-baseline.ps1`
- Modify when checks need new needles: `tools/check-ai-integration-020-engine-manifest.ps1`

- [x] Classify transient frame allocations, asset/package residency, renderer/RHI resources, editor/runtime UI resources, diagnostics buffers, and game-owned content memory.
- [x] Add AI-readable wording for allocator and lifetime boundaries: what is ready, what is planned, what is host-gated, and what must remain unsupported.
- [x] Define which future memory counters need to exist before allocator, pool, arena, or GPU residency changes can claim a performance improvement.
- [x] Keep CPU all-core usage, GPU memory residency, CUDA/HIP/SYCL, cross-vendor parity, and cross-backend parity unclaimed unless backed by focused recipes.

Phase 2 evidence on 2026-06-02: classified memory readiness into frame temporary, worker scratch, persistent CPU, package resident CPU, upload/staging, resident GPU, transient GPU, readback, and editor/tooling classes in `docs/specs/2026-06-01-engine-performance-optimization-foundation-v1.md`. AI-facing guidance in `docs/ai-game-development.md`, `docs/current-capabilities.md`, and `docs/rhi.md` now ties existing resident-byte, runtime residency budget, upload/staging, read-only RHI memory diagnostic, transient alias allocation, and `gpu_memory_policy_*` evidence to narrow ready claims while keeping global allocator replacement, allocator enforcement, all-core scheduling, NUMA/affinity tuning, SIMD dispatch, automatic/LRU GPU residency, CUDA/HIP/SYCL runtime paths, cross-vendor parity, cross-backend parity, and broad memory optimization unsupported.

## Phase 3 - Static Check And Next Implementation Wave Selection

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/01-one-dot-zero-readiness-ledger.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Generate: `engine/agent/manifest.json`
- Modify: relevant `tools/check-ai-integration*.ps1` and `tools/check-json-contracts*.ps1` files when new durable literals are added

- [ ] Add static checks only for durable AI-operable contract text, not every explanatory sentence.
- [ ] Select the next measured implementation wave after the taxonomy is complete, likely allocator diagnostics, transient-frame allocation counters, or job-system scheduling evidence depending on the audit.
- [ ] Record final validation evidence and hosted PR evidence.

## Validation Evidence

Close each behavior or contract slice with the smallest relevant focused checks first. A manifest/docs/static-check slice should normally include:

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`
- `git diff --check`

Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` at the slice-closing gate when C++/runtime/build/packaging/public contracts change, or when narrower checks cannot prove the changed surface.

Phase 1 validation on 2026-06-02:

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` - passed, no manifest drift retained.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` - passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` - passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` - passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` - passed.
- `git diff --check` - passed.
- Full `tools/validate.ps1` was not run for this Phase 1 slice because the change is limited to docs/agent-surface wording and the targeted checks cover the changed contracts.

Phase 2 validation on 2026-06-02:

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` - passed, no manifest drift retained.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` - passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` - passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` - passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` - passed.
- `git diff --check` - passed.
- Full `tools/validate.ps1` was not run for this Phase 2 slice because the change is limited to docs/agent-surface wording and existing manifest/static checks already cover the active memory lifetime contract.

## Done When

- AI-facing docs and manifests state the engine's ownership, lifetime, pointer, view, handle, and allocation boundaries.
- Public API guidance makes ownership transfer explicit and keeps raw pointers/references non-owning.
- Future allocator, job-system, NUMA, SIMD, GPU residency, CUDA/HIP/SYCL, and vendor-specific optimization work has measurable prerequisite counters or host-gated evidence rows.
- Unsupported broad CPU/GPU/memory optimization claims remain fail-closed until focused implementation and validation evidence exists.
