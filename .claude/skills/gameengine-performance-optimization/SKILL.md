---
name: gameengine-performance-optimization
description: Designs, implements, reviews, or debugs GameEngine performance and optimization work. Use for CPU/GPU/memory optimization, benchmarks, performance budgets, profiling evidence, job execution/topology/placement, scratch arenas, SIMD/ISA dispatch such as SSE2/AVX2/NEON, allocator or data-layout work, PGO/LTO/compiler flags, frame pacing, streaming IO, shader/pipeline cache, or any claim about speed, throughput, latency, memory, or optimized readiness.
paths:
  - "engine/**"
  - "games/**"
  - "docs/specs/**"
  - "docs/superpowers/plans/**"
  - "engine/agent/**"
  - "**/CMakeLists.txt"
  - "CMakePresets.json"
---

# GameEngine Performance Optimization

## Workflow

1. Scope the claim first: identify the exact subsystem, metric, host/backend, workload, fallback path, and unsupported adjacent claims. Never use a broad "optimized" claim when only a narrow lane is proven.
2. Read the current performance context from `docs/current-capabilities.md`, `docs/superpowers/plans/README.md`, `docs/specs/2026-06-01-engine-performance-optimization-foundation-v1.md`, and targeted `engine/agent/manifest.fragments/*.json` or `tools/agent-context.ps1 -ContextProfile Minimal|Standard`.
3. Prefer measurement, data layout, ownership clarity, and deterministic diagnostics before low-level tuning. Add budgets or package-visible evidence rows before claiming speedup.
4. Keep implementation slices narrow and clean breaking: one measured kernel, budget lane, scheduler policy, allocator boundary, package recipe, or backend proof per reviewable change.
5. Add or update tests for the smallest externally meaningful guarantee: scalar/reference equivalence, fallback selection, fail-closed diagnostics, evidence counters, deterministic ordering, or package-visible smoke output.
6. Update docs/plans/manifest fragments/static checks when public counters, validation recipes, package evidence, or AI-operable claims change. Compose `engine/agent/manifest.json` after fragment edits.
7. Validate with focused builds/tests/static checks while iterating. Use full `tools/validate.ps1` only for C++/runtime/build/packaging/public-contract slice gates or when narrower checks cannot prove the changed surface.

## Optimization Boundaries

- For CPU scheduling, keep policy, evidence, and execution separate: job graph/evidence before worker pool, topology before placement, host-independent placement before OS-specific CPU Sets or affinity.
- For memory work, classify lifetime and ownership before allocator replacement. Report bytes, counts, high-water marks, stale-generation, safe-point, cross-thread, false-sharing, and budget-pressure evidence.
- For SIMD/ISA work, keep scalar reference behavior, target-local translation units or OBJECT targets, target-local compile flags, runtime CPU/OS register gates, and package-visible selected-lane evidence. Do not set global `/arch:*`, `-mavx*`, PGO, LTO, or vendor flags for default validation.
- For GPU, renderer, streaming, or pipeline-cache work, keep backend evidence host-specific and do not transfer D3D12/Vulkan/Metal proof across backends without a reviewed parity row.
- For compiler optimization lanes, keep release-performance presets distinct from debug/editor/default validation. Record compiler, flags, profile collection/use commands, binary size, startup time, p95/p99 frame time, and changed crash/debug behavior.
- For generated-game optimization, express intent as schema-backed budgets, recipe evidence, or package counters. If the selected recipe cannot prove the claim, file an engine capability handoff instead of mutating engine internals.

## Do Not

- Do not broaden AGENTS.md, `.codex/rules`, or `.claude/settings.json` to make optimization faster; use narrower slices and focused validation.
- Do not add compatibility shims, deprecated aliases, duplicate APIs, or migration layers for greenfield optimization work unless a future release policy explicitly requires them.
- Do not claim broad CPU/GPU/memory optimization, all-core readiness, vendor parity, backend parity, async overlap, allocator enforcement, GPU residency, CUDA/HIP/SYCL, NEON, AVX-512, or PGO/LTO benefit without focused evidence for that exact claim.
- Do not expose native thread handles, affinity masks, RHI/native handles, or backend objects through public game APIs.
