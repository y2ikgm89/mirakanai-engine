# SIMD Dispatch Policy And Evidence v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` or focused inline execution. Steps use checkbox (`- [ ]`) syntax for tracking.

**Status:** Active.

**Local validation:** Complete; publication pending.

**Goal:** Add a narrow, vendor-neutral CPU SIMD dispatch evidence lane that lets AI-generated games distinguish scalar, SSE2, and AVX2-capable CPU math paths without claiming broad Intel/AMD optimization, GPU compute, CUDA, HIP, SYCL, or cross-vendor parity.

**Architecture:** `MK_core` owns a value-only SIMD capability and dispatch planner because CPU feature labels and math-kernel selection must not depend on OS, GPU, renderer, or editor modules. The selected first kernel is a bounded `float` dot-product evidence path over borrowed `std::span<const float>` inputs: scalar is always available, SSE2 is allowed on x86/x64 when the compiler target supports it, and AVX2 stays gated behind compile/runtime evidence rather than becoming a global build requirement. `sample_desktop_runtime_game` reports package-visible `simd_dispatch_policy_*` counters so generated games can record which CPU SIMD lane was selected while keeping raw pointers non-owning and native handles absent.

**Tech Stack:** C++23, `MK_core`, compiler intrinsics guarded by architecture macros, MSVC `__cpuid` / `__cpuidex` feature detection where available, `sample_desktop_runtime_game`, PowerShell validation wrappers.

## Official References

- Intel Intrinsics Guide: official searchable reference for SSE, SSE2, AVX, AVX2, AVX-512, AMX, and related intrinsics; its throughput and latency data comes from Intel 64 and IA-32 manuals.
- Intel 64 and IA-32 Architectures Optimization resources: current Intel optimization manuals, throughput/latency data, and Xeon tuning references for CPU-side vector optimization.
- Microsoft Learn `__cpuid` / `__cpuidex`: official MSVC intrinsics for reading CPU feature leaves; Intel and AMD document CPUID leaf naming differently, so engine labels must stay vendor-neutral.
- Microsoft Learn `/arch` x64: x64 defaults to SSE2, `/arch:AVX2` enables AVX2 code generation, and code must check CPU feature support before executing extension-specific instructions.
- AMD uProf and AMD Software Optimization Guide links: current AMD official profiling and Zen optimization references for evaluating bottlenecks and CPU feature behavior on AMD processors.
- ISO C++ paper P0214 / `std::experimental::simd` context: portable SIMD remains TS/C++26-adjacent rather than a C++23 baseline, so this slice must not require `std::simd` in public C++23 APIs.

## Context

- `unsupportedProductionGaps = []`; this is post-1.0 selected optimization evidence, not a new 1.0 blocker.
- Completed prerequisites: Performance Baseline v1, Memory Lifetime Taxonomy v1, Memory Diagnostics v1, Job Scheduling Evidence v1, Job Execution Worker Pool v1, Job Execution Placement Policy v1, Windows CPU Set Worker Placement v1, and Windows CPU Set SMT Worker Placement v1.
- The current optimization stack still explicitly leaves SIMD dispatch, CUDA/HIP/SYCL, GPU async overlap, allocator enforcement, automatic/LRU GPU residency, and broad CPU/GPU/memory optimization unclaimed.
- This slice answers the next CPU optimization gap because it is host-independent enough to validate locally and useful for both Intel and AMD x86/x64 CPUs without adding vendor-specific APIs.

## Constraints

- Keep public APIs value-only and dependency-free; raw pointers, `std::span`, and references are non-owning borrowed views.
- Do not add global `/arch:AVX2`, `/arch:AVX512`, or per-host build assumptions to the whole engine.
- Do not execute AVX/AVX2 code unless both compile-time availability and runtime CPU/OS support are proven.
- Do not claim broad Intel/AMD tuning, ARM NEON, Linux affinity, NUMA placement, GPU async overlap, CUDA, HIP, SYCL, GPU residency, or cross-vendor performance parity.
- Keep tests deterministic and tolerant of hosts where only scalar or SSE2 evidence is available.

## Files

- Modify: `engine/core/include/mirakana/core/*.hpp` or an existing core public header selected after code inspection.
- Modify: `engine/core/src/*.cpp` or a focused new core source if current layout supports it.
- Modify: `tests/unit/core_tests.cpp` or the smallest existing `MK_core` unit-test file that owns numeric utility evidence.
- Modify: `games/sample_desktop_runtime_game/main.cpp`.
- Modify: `games/sample_desktop_runtime_game/game.agent.json`.
- Modify: `games/CMakeLists.txt`.
- Modify: `tools/validate-installed-desktop-runtime.ps1`.
- Modify: `docs/current-capabilities.md`, `docs/roadmap.md`, `docs/superpowers/plans/README.md`, this plan, manifest fragments, and composed `engine/agent/manifest.json`.
- Modify static checks only for durable new public literals and package-visible `simd_dispatch_policy_*` fields.

## Task 1: Core SIMD Capability And Dispatch Contract

- [x] Add RED tests for scalar-only dispatch, x86/x64 SSE2-capable dispatch labels, AVX2 unsupported diagnostics when compile/runtime gates are missing, and stable dot-product results across the selected lane.
- [x] Add dependency-free public rows for SIMD feature labels, dispatch status, diagnostics, selected lane, requested lane, input count, scalar fallback, and deterministic numeric result evidence.
- [x] Implement runtime feature probing behind compiler/architecture guards; non-x86 hosts must report scalar-ready without failing the engine.
- [x] Implement the scalar dot-product evidence kernel with `std::span<const float>` and no ownership transfer.
- [x] Implement the SSE2 evidence kernel only behind architecture guards where the compiler target supports it; keep AVX2 planned/gated unless a local per-target compile flag is added safely.
- [x] Run focused `MK_core` tests.

## Task 2: Package Evidence

- [x] Add `sample_desktop_runtime_game --require-simd-dispatch-policy`.
- [x] Emit `simd_dispatch_policy_status`, `simd_dispatch_policy_ready`, `simd_dispatch_policy_selected_lane`, `simd_dispatch_policy_scalar_fallback`, input/result counters, diagnostics, and explicit zero side-effect flags for CUDA/HIP/SYCL/GPU/NUMA/native handles.
- [x] Update source-tree and installed desktop runtime validation to require the new package-visible fields.
- [x] Run desktop-runtime source-tree smoke and installed package smoke.

## Task 3: Docs, Manifest, Static Guards

- [x] Update capabilities, roadmap, plan registry, manifest fragments, and static checks for the ready boundary and non-claims.
- [x] Compose `engine/agent/manifest.json`.
- [x] Run `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, and `tools/check-agents.ps1`.

## Task 4: Slice Validation And Publication

- [x] Run `tools/check-public-api-boundaries.ps1`.
- [x] Run `tools/check-format.ps1`.
- [x] Run targeted `tools/check-tidy.ps1 -Files` for changed C++ files and sample file.
- [x] Run full `tools/validate.ps1`.
- [x] Run `tools/check-publication-preflight.ps1 -Branch codex/simd-dispatch-policy-v1`.
- [x] Commit task-owned files with validation evidence.
- [ ] Push branch, create PR, wait for hosted checks including `PR Gate`, merge with `--match-head-commit`, verify `origin/main` contains the head, and run merged-worktree cleanup.

## Validation Evidence

- RED: changing the AVX2 diagnostic expectation to `reviewed_target_gate_missing` failed until the new SIMD diagnostic enum and policy gate were implemented.
- Focused core: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_core_tests`.
- Source-tree desktop runtime: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset desktop-runtime`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset desktop-runtime --target sample_desktop_runtime_game`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset desktop-runtime --output-on-failure -R sample_desktop_runtime_game_smoke`.
- Direct smoke evidence: `simd_dispatch_policy_status=ready`, `simd_dispatch_policy_ready=1`, `simd_dispatch_policy_requested_lane=auto_select`, `simd_dispatch_policy_selected_lane=sse2`, `simd_dispatch_policy_dot_product_result=120`, `simd_dispatch_policy_avx2_selected=0`, borrowed span inputs, no retained raw pointers, and zero native handle / NUMA / GPU async-overlap / CUDA / HIP / SYCL side effects.
- Agent/static: `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, and `tools/check-agents.ps1`.
- Installed package smoke: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game` passed and validated the same `simd_dispatch_policy_*` installed-runtime counters.
- Quality gates: `tools/check-public-api-boundaries.ps1`; `tools/check-format.ps1`; `tools/check-tidy.ps1 -Files engine/core/src/simd_dispatch.cpp,tests/unit/core_tests.cpp`; `tools/check-tidy.ps1 -Preset desktop-runtime -Files games/sample_desktop_runtime_game/main.cpp`.
- Full local validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed; diagnostic-only host gates remained Apple/Metal host availability as expected on Windows.
- Publication preflight: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1 -Branch codex/simd-dispatch-policy-v1` passed with GitHub network/auth ready and no index lock blocker.
- Commit: task-owned files committed with validation evidence.

## Done When

- `MK_core` can choose and report a deterministic scalar/SSE2 SIMD dispatch lane without broad CPU-vendor or GPU claims.
- The selected package smoke reports `simd_dispatch_policy_*` evidence and zero native handle, GPU, CUDA, HIP, SYCL, NUMA, and broad optimization side effects.
- Docs, manifest, and static checks distinguish narrow CPU SIMD dispatch evidence from broad Intel/AMD tuning, CUDA/HIP/SYCL, GPU async overlap, automatic/LRU residency, allocator enforcement, and cross-vendor parity.
- Focused checks, package smoke, full local validation, hosted PR checks, merge, and cleanup are complete.
