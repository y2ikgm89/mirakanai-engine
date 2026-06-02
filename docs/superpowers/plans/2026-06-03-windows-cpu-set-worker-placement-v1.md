# Windows CPU Set Worker Placement v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` or focused inline execution. Steps use checkbox (`- [ ]`) syntax for tracking.

**Status:** Completed.

**Goal:** Connect completed host-independent job placement policy evidence to a Windows CPU Sets worker-placement lane without exposing native handles or weakening the OS-independent `MK_core` boundary.

**Architecture:** `MK_core` gains a value-only worker-start placement callback hook that runs inside each worker thread before queue processing. `MK_platform_win32` owns the Windows SDK CPU Sets adapter: query `SYSTEM_CPU_SET_INFORMATION`, select per-worker CPU Set ids, and apply them with `SetThreadSelectedCpuSets(GetCurrentThread(), ...)`. Sample package evidence proves the selected Windows path while Linux affinity, NUMA allocation, hybrid/SMT execution, SIMD, GPU async overlap, CUDA/HIP/SYCL, cross-vendor parity, and broad optimization remain unclaimed.

**Tech Stack:** C++23, `MK_core`, `MK_platform_win32`, Windows SDK `processthreadsapi.h` / `winnt.h`, `sample_desktop_runtime_game`, PowerShell validation wrappers.

## Official References

- Microsoft Learn: `SetThreadSelectedCpuSets` sets a selected CPU Set assignment for a thread, accepts `GetCurrentThread`, and is available on Windows 10 / Windows Server 2016 and later.
- Microsoft Learn: `GetThreadSelectedCpuSets` queries explicit CPU Set assignments and returns `ERROR_INSUFFICIENT_BUFFER` when the caller-provided buffer is too small.
- Microsoft Learn: `GetSystemCpuSetInformation` enumerates available CPU Sets using a caller-sized buffer and `GetCurrentProcess` for `AllocatedToTargetProcess` evidence.
- Microsoft Learn: `SYSTEM_CPU_SET_INFORMATION` is variable-sized; iteration must use `Size`, and relevant fields include `Id`, `Group`, `LogicalProcessorIndex`, `CoreIndex`, `NumaNodeIndex`, `EfficiencyClass`, and parked/allocation flags.

## Context

- `unsupportedProductionGaps = []`; this is future selected optimization work, not a new 1.0 blocker.
- Completed prerequisites: Job Scheduling Evidence v1, Job Execution Worker Pool v1, Job Execution Topology Policy v1, Job Execution Work Stealing v1, and Job Execution Placement Policy v1.
- Current package evidence intentionally reports zero affinity/NUMA/SIMD/GPU/CUDA/HIP/SYCL execution side effects. This slice may promote only the Windows CPU Sets worker-placement counter.

## Constraints

- Keep `engine/core` standard-library-only and independent from OS APIs, native handles, renderer, editor, and platform modules.
- Do not expose `HANDLE`, `GROUP_AFFINITY`, CPU masks, or backend-private objects through public generated-game APIs.
- The core callback must be value-only, deterministic in results, and safe when omitted.
- Windows adapter code stays under `engine/platform/win32`; non-Windows behavior remains an explicit non-claim.
- No compatibility shims or broad optimization claims.

## Files

- Modify: `engine/core/include/mirakana/core/job_execution.hpp`
- Modify: `engine/core/src/job_execution.cpp`
- Modify: `tests/unit/core_tests.cpp`
- Create: `engine/platform/win32/include/mirakana/platform/win32/win32_cpu_sets.hpp`
- Create: `engine/platform/win32/src/win32_cpu_sets.cpp`
- Modify: `engine/platform/win32/CMakeLists.txt`
- Modify: `tests/unit/win32_platform_tests.cpp`
- Modify: `games/sample_desktop_runtime_game/main.cpp`
- Modify: `games/sample_desktop_runtime_game/game.agent.json`
- Modify: `games/CMakeLists.txt`
- Modify: `tools/validate-installed-desktop-runtime.ps1`
- Modify: `docs/current-capabilities.md`, `docs/roadmap.md`, `docs/superpowers/plans/README.md`, this plan, and `docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`, `engine/agent/manifest.fragments/004-modules.json`, `engine/agent/manifest.fragments/015-aiDrivenGameWorkflow.json`, and composed `engine/agent/manifest.json`
- Modify static checks only for new durable literals/counters that need enforcement.

## Task 1: Core Worker Placement Hook

- [x] Add focused RED tests in `tests/unit/core_tests.cpp` proving a worker-placement callback runs exactly once per worker before task execution and that `JobExecutionRunResult` reports attempted/applied/diagnostic counts.
- [x] Add value-only public API in `job_execution.hpp`: request/result rows, callback type, callback field on `JobExecutionPoolDesc`, result counters, and label helpers if needed.
- [x] Implement callback invocation in `JobExecutionPool::Impl::run_worker` before queue processing without data races.
- [x] Keep omitted callback behavior identical to existing pools.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_core_tests`.

**Task 1 Evidence:** RED first failed on the missing worker-placement API. GREEN passed with `MK_core_tests` focused build and CTest after adding the value-only callback hook and worker placement run counters.

## Task 2: Windows CPU Sets Adapter

- [x] Add RED tests in `tests/unit/win32_platform_tests.cpp` for synthetic CPU Set row selection: skip parked/allocated-to-other-process rows, prefer higher `EfficiencyClass` for performance mode, lower `EfficiencyClass` for efficiency mode, and preserve stable worker-to-set selection.
- [x] Add `win32_cpu_sets.hpp/.cpp` with value rows, topology query, selection plan, and a `JobExecutionWorkerPlacementCallback` builder that applies selected ids to the current worker thread.
- [x] Use `GetSystemCpuSetInformation` with the documented two-call buffer sizing pattern and variable-size structure iteration by `Size`.
- [x] Use `SetThreadSelectedCpuSets(GetCurrentThread(), ids.data(), count)` for the actual worker-thread placement path and keep any `GetThreadSelectedCpuSets` verification private/value-only.
- [x] Add the source to `engine/platform/win32/CMakeLists.txt`, linking `MK_core` publicly or privately as required by the public callback builder surface.
- [x] Run focused `MK_win32_platform_tests`.

**Task 2 Evidence:** RED first failed on the missing `mirakana/platform/win32/win32_cpu_sets.hpp`. GREEN passed with `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_win32_platform_tests` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_win32_platform_tests`.

## Task 3: Package-Visible Windows Evidence

- [x] Add `sample_desktop_runtime_game --require-windows-cpu-set-worker-placement`.
- [x] Route the flag through the existing job execution foundation/topology/work-stealing/placement evidence path so the sample starts a real worker pool with the Win32 CPU Set callback.
- [x] Emit package counters: status, ready, topology row count, selected CPU Set count, worker placement attempts, applied count, diagnostics, `native_thread_handles_exposed=0`, and explicit zero Linux affinity / NUMA allocation / hybrid-SMT / SIMD / GPU async / CUDA / HIP / SYCL side effects.
- [x] Update `tools/validate-installed-desktop-runtime.ps1`, `games/CMakeLists.txt`, and `game.agent.json` package recipes for selected Windows package smoke.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game`.

**Task 3 Evidence:** `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset desktop-runtime`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset desktop-runtime --output-on-failure -R sample_desktop_runtime_game_smoke`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game` passed. Installed package validation reported `windows_cpu_set_worker_placement_status=ready`, `windows_cpu_set_worker_placement_ready=1`, `windows_cpu_set_worker_placement_attempts=2`, `windows_cpu_set_worker_placement_applied=2`, positive `windows_cpu_set_worker_placement_selected_cpu_sets`, `windows_cpu_set_worker_placement_native_thread_handles_exposed=0`, and zero Linux affinity / NUMA allocation / hybrid-SMT / SIMD / GPU async / CUDA / HIP / SYCL side-effect counters.

## Task 4: Docs, Manifest, Static Guards

- [x] Update capabilities, roadmap, backlog, registry, and this plan to describe the Windows CPU Sets ready boundary and non-claims.
- [x] Update manifest fragments and compose `engine/agent/manifest.json`.
- [x] Add or adjust static-check needles for new counters and non-claims only where durable.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` if agent-facing guidance changes.

**Task 4 Evidence:** `tools/compose-agent-manifest.ps1 -Write`, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, and `tools/check-agents.ps1` passed after returning `currentActivePlan` to the production-completion master selection gate and recording Windows CPU Set Worker Placement v1 as completed narrow Windows evidence.

## Task 5: Slice Validation And Publication

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run targeted `tools/check-tidy.ps1 -Files` for changed C++ files and `-Preset desktop-runtime` for sample-only files not present in `dev`.
- [x] Run full `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1 -Branch codex/windows-cpu-set-placement-v1`.
- [x] Commit task-owned files with validation evidence.
- [ ] Push branch, create PR, wait for hosted checks including `PR Gate`, merge with `--match-head-commit`, verify `origin/main` contains the head, and run `tools/remove-merged-worktree.ps1`.

**Task 5 Evidence:** `tools/check-public-api-boundaries.ps1`, `tools/check-format.ps1`, changed C++ `tools/check-tidy.ps1 -Files`, sample `tools/check-tidy.ps1 -Preset desktop-runtime -Files games/sample_desktop_runtime_game/main.cpp`, focused `MK_core_tests`, focused `MK_win32_platform_tests`, desktop-runtime source-tree smoke, `tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game`, full `tools/validate.ps1`, and `tools/check-publication-preflight.ps1 -Branch codex/windows-cpu-set-placement-v1` passed. `win32_cpu_sets.cpp` tidy warnings were resolved by switching to `std::ranges` algorithms before final validation. The commit records the validated implementation checkpoint.

## Done When

- Core worker pools can optionally report worker-start placement attempts without OS dependencies.
- `MK_platform_win32` can query and apply Windows CPU Set selection for worker threads using official Windows SDK APIs.
- `sample_desktop_runtime_game` installed package validation proves selected Windows CPU Set worker placement evidence.
- Manifest/docs/static checks distinguish this from Linux affinity, NUMA allocation execution, hybrid/SMT execution, SIMD, GPU async, CUDA/HIP/SYCL, cross-vendor parity, and broad optimization.
- Focused checks, package smoke, full local validation, hosted PR checks, merge, and cleanup are complete.
