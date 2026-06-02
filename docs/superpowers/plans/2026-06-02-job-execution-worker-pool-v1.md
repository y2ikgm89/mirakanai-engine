# Job Execution Worker Pool v1 - 2026-06-02

**Goal:** Add a first-party C++23 worker-thread execution pool on top of completed Job Scheduling Evidence v1 so selected engine and sample paths can prove real worker threads, bounded queue execution, cooperative shutdown, and diagnostic evidence without claiming affinity, NUMA placement, SIMD dispatch, work stealing, or broad all-core optimization.

**Architecture:** Keep the implementation in `MK_core`; use standard C++ RAII threads and synchronization only. Phase 1 starts a bounded persistent worker pool with explicit worker count, RAII-owned `std::thread` workers, cooperative stop, bounded worker queues, `execute(batch)` drain semantics, fail-closed execution diagnostics, worker-local scratch, and `JobSchedulingExecutionEvidence` mapping. Phase 2 adds package counters. Later topology, affinity, NUMA, SIMD, and GPU-overlap policies require separate focused plans.

**Tech Stack:** C++23 `MK_core`, RAII-owned `std::thread`, `JobExecutionStopToken`, `std::mutex`, `std::condition_variable`, existing `ScratchArena`, `JobWorkerTopologyRow`, `JobQueueCounterRow`, `JobSchedulingWorkItemRow`, `JobSchedulingExecutionEvidence`, PowerShell 7 validation wrappers, composed `engine/agent/manifest.json`, and static checks under `tools/check-ai-integration*.ps1` / `tools/check-json-contracts*.ps1`.

---

**Plan ID:** `job-execution-worker-pool-v1`

**Status:** Completed.

Selected on 2026-06-02 after [Job Scheduling Evidence v1](2026-06-02-job-scheduling-evidence-v1.md) completed Phase 3 package-visible scheduler evidence.

Completed through PR #381 / merge commit `8c6a56f8` with full validation and package-visible `job_execution_foundation_*` evidence.

## Official Guidance

- Microsoft STL documents `std::jthread` as the preferred auto-joining cooperative-stop primitive, but hosted AppleClang/libc++ currently does not expose it in this CI lane; Phase 1 therefore uses first-party `JobExecutionStopToken` plus RAII-owned `std::thread` workers to preserve the same engine contract across supported hosted builds.
- Microsoft C++ docs confirm `std::condition_variable` / condition-variable waits as the standard waiting primitive for mutex-protected events.
- Microsoft multicore game guidance recommends persistent worker pools rather than frequent thread create/destroy, bounded work queues, minimized synchronization, and avoiding Windows affinity generalization unless topology evidence justifies it.
- Intel and AMD optimization references remain inputs for later topology, SMT, NUMA, cache, and SIMD phases; this plan deliberately does not pin threads or claim vendor-specific tuning.

## Constraints

- Keep `engine/core` independent from OS APIs, GPU APIs, editor code, renderer/RHI, and asset formats.
- Require explicit `worker_count`; do not infer all-core readiness or auto-pin threads.
- Do not add work stealing, affinity, NUMA, hybrid CPU, SMT, SIMD, CUDA/HIP/SYCL, GPU async overlap, or vendor-specific policy in Phase 1.
- Keep queue capacity bounded and fail closed on invalid configuration, invalid tasks, queue overflow, thrown tasks, and stopped-pool execution.
- Keep package-visible counters as a later phase unless the sample path can honestly exercise the real pool.
- Do not expose fire-and-forget futures, recursive submit/wait, shared diagnostics mutation from worker bodies, or OS completion order as deterministic evidence in Phase 1.

## Phase 0 - Selection And Contract

**Files:**
- Create: `docs/superpowers/plans/2026-06-02-job-execution-worker-pool-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: static checks if active-plan literals change

- [x] Select this plan as `currentActivePlan` and `recommendedNextPlan.id = job-execution-worker-pool-v1`.
- [x] Keep `unsupportedProductionGaps = []` and describe this as an optimization follow-up, not an Engine 1.0 blocker.
- [x] Record official guidance and explicit non-claims.

## Phase 1 - Core Worker Pool Execution

**Files:**
- Create: `engine/core/include/mirakana/core/job_execution.hpp`
- Create: `engine/core/src/job_execution.cpp`
- Modify: `engine/core/CMakeLists.txt`
- Modify: `tests/unit/core_tests.cpp`

- [x] Add `JobExecutionPool` as an RAII non-copyable `MK_core` type using RAII-owned `std::thread` workers.
- [x] Add explicit `JobExecutionPoolDesc` with non-zero worker count and per-worker queue capacity.
- [x] Add `JobExecutionBatchDesc`, `JobExecutionTaskDesc`, `JobExecutionContext`, `JobExecutionStopToken`, `execute(batch)`, `request_stop`, `stop_and_drain`, `status`, and result/evidence APIs.
- [x] Execute batch tasks on worker threads and expose actual worker thread evidence.
- [x] Reject invalid tasks, stopped execution, queue overflow, dependency cycles, and blocked dependencies without blocking unboundedly.
- [x] Catch task exceptions and record fail-closed diagnostics without sharing caller diagnostics across workers.
- [x] Map actual execution into existing `JobSchedulingExecutionEvidence` rows for topology, bounded queues, deterministic publish order, deterministic merges, worker waits, and worker scratch counters.
- [x] Keep no affinity/NUMA/SIMD/GPU-overlap side effects in this phase.

## Phase 2 - Package Evidence Mapping

**Files:**
- Modify: selected sample package path only if it can honestly exercise the real pool
- Modify: selected `game.agent.json` / validators only when installed package validation gains real pool counters
- Modify: docs/manifest/static checks for durable counter names

- [x] Add package-visible `job_execution_foundation_*` counters only after Phase 1 is green.
- [x] Preserve previous `job_scheduling_evidence_*` rows as value-only evidence.
- [x] Keep broad all-core and vendor-specific optimization unsupported until measured follow-up plans land.

**Phase 2 Evidence:** `sample_desktop_runtime_game --require-job-execution-foundation` now runs the real `MK_core` `JobExecutionPool` with two RAII-owned `std::thread` workers, three dependency-ordered task bodies, worker-local `ScratchArena` leases, deterministic merge evidence, and package-visible `job_execution_foundation_*` counters. The existing `job_scheduling_evidence_*` rows remain value-only and still report zero native-thread/thread-pool/affinity/NUMA/SIMD/GPU-overlap side-effect counters. New `job_execution_foundation_*` rows report ready pool/run status, two worker threads, three submitted/executed task bodies, positive worker scratch bytes/high-water, and zero work-stealing, affinity, NUMA, SIMD, GPU async-overlap, CUDA, HIP, and SYCL side-effect counters.

**Phase 2 Focused Validation:**

- TDD red: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset desktop-runtime --output-on-failure -R sample_desktop_runtime_game_smoke` failed on unknown `--require-job-execution-foundation` before implementation.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset desktop-runtime`: pass.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset desktop-runtime --target sample_desktop_runtime_game`: pass.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset desktop-runtime --output-on-failure -R sample_desktop_runtime_game_smoke`: pass.
- Direct smoke output from `out/build/desktop-runtime/games/Debug/sample_desktop_runtime_game/sample_desktop_runtime_game.exe --smoke --require-job-scheduling-evidence --require-job-execution-foundation` reported `job_execution_foundation_status=ready`, `job_execution_foundation_ready=1`, `job_execution_foundation_worker_threads_started=2`, `job_execution_foundation_tasks_submitted=3`, `job_execution_foundation_tasks_executed=3`, `job_execution_foundation_task_side_effects=3`, `job_execution_foundation_deterministic_merges=3`, positive `job_execution_foundation_scratch_bytes` / `job_execution_foundation_scratch_high_water_bytes`, and zero work-stealing/affinity/NUMA/SIMD/GPU-overlap/CUDA/HIP/SYCL side-effect counters.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game`: pass; installed D3D12 package smoke reported the same `job_execution_foundation_*` counters and `installed-desktop-runtime-validation: ok`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Preset desktop-runtime -Configuration Debug -ReuseExistingFileApiReply -Files games/sample_desktop_runtime_game/main.cpp`: pass.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: pass.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1`: pass.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: pass.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: pass.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`: pass.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: pass.

## Validation Evidence

Close each behavior or contract slice with focused checks first. Phase 1 should include:

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev`
- TDD red for `MK_core_tests` before the worker-pool API exists.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_core_tests`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files 'engine/core/src/job_execution.cpp,tests/unit/core_tests.cpp'`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`

## Done When

- `MK_core` exposes a real, bounded, RAII worker-thread pool with cooperative shutdown, batch drain semantics, worker-local scratch, and deterministic diagnostic evidence.
- Tests prove worker execution, queue overflow rejection, stopped-pool rejection, exception diagnostics, dependency ordering, deterministic publish order, worker scratch isolation, and evidence rows.
- Agent/docs/manifest/static checks distinguish real thread-pool execution from still-unclaimed all-core, affinity, NUMA, SIMD, GPU async-overlap, CUDA/HIP/SYCL, and broad optimization readiness.
- Relevant focused checks and full validation pass, or concrete host/tool blockers are recorded.
