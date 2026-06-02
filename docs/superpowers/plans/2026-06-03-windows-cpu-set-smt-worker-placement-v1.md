# Windows CPU Set SMT Worker Placement v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` or focused inline execution. Steps use checkbox (`- [ ]`) syntax for tracking.

**Status:** Completed through PR #393 / merge commit `28ec2a3f99896df1f62639944e54f1376a41eb99`.

**Goal:** Promote the completed Windows CPU Set worker-placement foundation into a selected SMT-aware worker placement lane that prefers distinct `CoreIndex` rows before CPU Sets that share significant execution resources, while still hiding native handles and avoiding broad CPU optimization claims.

**Architecture:** `MK_platform_win32` extends its CPU Sets placement planner for the existing `JobExecutionPlacementPolicyMode::avoid_smt_siblings` mode. The planner uses official Windows CPU Set topology rows: `CoreIndex` is the same for CPU Sets that share significant execution resources such as SMT hardware threads. `MK_core` remains value-only and OS-independent; it already carries the selected placement mode and worker-start callback result counters. `sample_desktop_runtime_game` adds a selected package evidence flag that starts a real worker pool with the SMT-aware Win32 callback and emits package-visible counters for distinct core selection, SMT sibling deferral, applied worker placement, and explicit non-claims for hybrid P/E-core pinning, Linux affinity, NUMA allocation, SIMD, GPU async, CUDA, HIP, and SYCL.

**Tech Stack:** C++23, `MK_core`, `MK_platform_win32`, Windows SDK CPU Sets APIs, `sample_desktop_runtime_game`, PowerShell validation wrappers.

## Official References

- Microsoft Learn: CPU Sets are a soft affinity API compatible with OS power management, map CPU Set IDs to virtual processor affinities, respect restrictive affinity masks first, and allow thread-selected assignments.
- Microsoft Learn: `SYSTEM_CPU_SET_INFORMATION` is variable-sized; iterate with `Size`. `CoreIndex` is equal for CPU Sets sharing significant execution resources such as SMT hardware threads, and `Id` is the value passed to `SetThreadSelectedCpuSets`.
- Microsoft Learn: `SetThreadSelectedCpuSets(GetCurrentThread(), ids, count)` sets the thread-selected CPU Set assignment and overrides process defaults.
- Microsoft Learn: `GetThreadSelectedCpuSets(GetCurrentThread(), ...)` queries explicit thread CPU Set assignments for verification.
- Intel: Performance hybrid architecture guidance separates core-type selection from SMT sibling behavior; this slice uses only the Windows `CoreIndex` SMT evidence boundary.
- AMD: AMD uProf official documentation links current AMD Software Optimization Guides for Zen families; this slice records SMT relevance without using AMD-specific native APIs or claiming AMD parity.

## Context

- `unsupportedProductionGaps = []`; this is post-1.0 selected optimization evidence, not a new 1.0 blocker.
- Completed prerequisites: Job Execution Placement Policy v1 and Windows CPU Set Worker Placement v1.
- Local host preflight found 32 CPU Sets, repeated `CoreIndex` pairs, and one `EfficiencyClass`. That supports SMT sibling avoidance evidence but not hybrid P/E-core execution evidence.
- Current package evidence reports `windows_cpu_set_worker_placement_hybrid_smt_policy_applied=0`. This slice must not promote that hybrid/SMT combined counter; it adds a separate SMT-only counter.

## Constraints

- Keep `engine/core` independent from OS APIs and native handles.
- Do not expose `HANDLE`, `GROUP_AFFINITY`, CPU masks, or backend-private objects through public generated-game APIs.
- Do not claim hybrid P/E-core pinning, Linux affinity, NUMA allocation execution, broad SMT scheduling, SIMD dispatch, GPU async overlap, CUDA, HIP, SYCL, cross-vendor parity, or broad CPU/GPU/memory optimization.
- The planner must be deterministic for synthetic rows and stable when all CPU Sets are single-threaded or homogeneous.
- The execution path must use the existing worker-start callback and verify assignments privately.

## Files

- Modify: `engine/platform/win32/include/mirakana/platform/win32/win32_cpu_sets.hpp`
- Modify: `engine/platform/win32/src/win32_cpu_sets.cpp`
- Modify: `tests/unit/win32_platform_tests.cpp`
- Modify: `games/sample_desktop_runtime_game/main.cpp`
- Modify: `games/sample_desktop_runtime_game/game.agent.json`
- Modify: `games/CMakeLists.txt`
- Modify: `tools/validate-installed-desktop-runtime.ps1`
- Modify: `docs/current-capabilities.md`, `docs/roadmap.md`, `docs/superpowers/plans/README.md`, this plan, and the production completion backlog row if needed.
- Modify: `engine/agent/manifest.fragments/004-modules.json`, `010-aiOperableProductionLoop.json`, `014-gameCodeGuidance.json`, `015-aiDrivenGameWorkflow.json`, and composed `engine/agent/manifest.json`.
- Modify static checks only for new durable literals/counters that need enforcement.

## Task 1: Win32 CPU Set SMT Planner

- [x] Add RED tests proving `avoid_smt_siblings` filters unavailable rows, assigns distinct `(group, core_index)` rows before sibling CPU Sets, and records distinct-core / sibling / policy evidence counters.
- [x] Extend `Win32CpuSetWorkerPlacementPlan` with deterministic counters: distinct core count, SMT sibling CPU Set count, SMT topology known, and SMT sibling policy applied.
- [x] Keep existing `prefer_performance_cores` and `prefer_efficiency_cores` behavior stable for existing tests.
- [x] Run focused `MK_win32_platform_tests`.

## Task 2: Package Evidence

- [x] Add `sample_desktop_runtime_game --require-windows-cpu-set-smt-worker-placement`.
- [x] Build evidence through `select_job_execution_placement_policy(... avoid_smt_siblings ...)` using SMT evidence derived from the CPU Set rows.
- [x] Start a real `JobExecutionPool` with the Win32 CPU Set callback and selected `avoid_smt_siblings` mode.
- [x] Emit package counters for status, ready, selected mode, selected CPU Sets, distinct cores, SMT sibling CPU Sets, SMT topology known, attempts/applied, and side-effect non-claims.
- [x] Update installed runtime validation and package recipes.
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
- [x] Run `tools/check-publication-preflight.ps1 -Branch codex/windows-cpu-set-smt-placement-v1`.
- [x] Commit task-owned files with validation evidence.
- [x] Push branch, create PR #393, wait for hosted checks including `PR Gate`, merge with `--match-head-commit`, verify `origin/main` contains the head, and run merged-worktree cleanup.

## Closeout Evidence

- Local validation: focused Win32 platform tests, desktop-runtime source-tree smoke, installed desktop runtime package smoke, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-agents.ps1`, `tools/check-public-api-boundaries.ps1`, `tools/check-format.ps1`, targeted `tools/check-tidy.ps1`, `tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game`, and full `tools/validate.ps1` passed before publication.
- Hosted validation: PR #393 completed `Agent Static Guards`, CodeQL, Linux CMake, Linux Coverage, Linux Clang ASan/UBSan, macOS Metal CMake, iOS Simulator smoke, Windows MSVC, full static analysis shards, and `PR Gate`.
- Merge evidence: PR #393 merged at `2026-06-02T20:12:08Z` as merge commit `28ec2a3f99896df1f62639944e54f1376a41eb99`; `origin/main..715c1acee13c596bd93d541bb6e2c441026d8b77` was empty after sync.

## Done When

- Win32 CPU Set placement can select distinct physical cores before SMT sibling CPU Sets for `avoid_smt_siblings`.
- The selected package smoke proves real worker-start CPU Set application with SMT policy evidence and no native handle exposure.
- Docs, manifest, and static checks distinguish this from hybrid P/E-core pinning, Linux affinity, NUMA allocation execution, broad SMT scheduling, SIMD, GPU async, CUDA/HIP/SYCL, cross-vendor parity, and broad optimization.
- Focused checks, package smoke, full local validation, hosted PR checks, merge, and cleanup are complete.
