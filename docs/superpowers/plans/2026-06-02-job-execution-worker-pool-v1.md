# Job Execution Worker Pool v1 - 2026-06-02

**Goal:** Add a first-party C++23 worker-thread execution pool on top of completed Job Scheduling Evidence v1 so selected engine and sample paths can prove real worker threads, bounded queue execution, cooperative shutdown, and diagnostic evidence without claiming affinity, NUMA placement, SIMD dispatch, work stealing, or broad all-core optimization.

**Architecture:** Keep the implementation in `MK_core`; use standard C++ RAII threads and synchronization only. Phase 1 starts a bounded persistent worker pool with explicit worker count, `std::jthread` auto-join/cooperative stop, bounded worker queues, `execute(batch)` drain semantics, fail-closed execution diagnostics, worker-local scratch, and `JobSchedulingExecutionEvidence` mapping. Later phases may add package counters and topology policy, but only after focused evidence.

**Tech Stack:** C++23 `MK_core`, `std::jthread`, `std::stop_token`, `std::mutex`, `std::condition_variable_any`, existing `ScratchArena`, `JobWorkerTopologyRow`, `JobQueueCounterRow`, `JobSchedulingWorkItemRow`, `JobSchedulingExecutionEvidence`, PowerShell 7 validation wrappers, composed `engine/agent/manifest.json`, and static checks under `tools/check-ai-integration*.ps1` / `tools/check-json-contracts*.ps1`.

---

**Plan ID:** `job-execution-worker-pool-v1`

**Status:** Active.

Selected on 2026-06-02 after [Job Scheduling Evidence v1](2026-06-02-job-scheduling-evidence-v1.md) completed Phase 3 package-visible scheduler evidence.

## Official Guidance

- Context7 `/microsoft/stl` confirms `std::jthread` auto-joins on destruction and supports cooperative cancellation with `std::stop_token`.
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

- [x] Add `JobExecutionPool` as an RAII non-copyable `MK_core` type using `std::jthread`.
- [x] Add explicit `JobExecutionPoolDesc` with non-zero worker count and per-worker queue capacity.
- [x] Add `JobExecutionBatchDesc`, `JobExecutionTaskDesc`, `JobExecutionContext`, `execute(batch)`, `request_stop`, `stop_and_drain`, `status`, and result/evidence APIs.
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

- [ ] Add package-visible `job_execution_foundation_*` counters only after Phase 1 is green.
- [ ] Preserve previous `job_scheduling_evidence_*` rows as value-only evidence.
- [ ] Keep broad all-core and vendor-specific optimization unsupported until measured follow-up plans land.

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
