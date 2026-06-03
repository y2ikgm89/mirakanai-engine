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

# GameEngine performance optimization (Cursor)

Full workflow lives in shared skills. Read these canonical files (ASCII paths):

| Layer | Path |
| --- | --- |
| Claude Code | `.claude/skills/gameengine-performance-optimization/SKILL.md` |
| Codex | `.agents/skills/performance-optimization-change/SKILL.md` |
| Baseline | `AGENTS.md` |

Before claiming speedup, optimized readiness, CPU/GPU/memory parity, SIMD/ISA readiness, or compiler-flag benefit, read the full shared skill and the current status in `docs/current-capabilities.md`, `docs/superpowers/plans/README.md`, and `docs/specs/2026-06-01-engine-performance-optimization-foundation-v1.md`.
