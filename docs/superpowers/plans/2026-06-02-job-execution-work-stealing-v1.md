# Job Execution Work Stealing v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a narrow, first-party, opt-in `MK_core` work-stealing execution path for `JobExecutionPool` with package-visible evidence, without claiming affinity, NUMA placement, SMT/hybrid CPU policy, SIMD dispatch, GPU async overlap, CUDA/HIP/SYCL, or broad all-core optimization.

**Architecture:** Keep the current one-queue-per-worker topology and deterministic scheduling evidence. When `JobExecutionPoolDesc::work_stealing_enabled` is true and more than one worker exists, an idle worker first checks its own queue, then attempts to steal from other worker queues, records per-worker attempt/success/wait counters, and publishes those counters into the existing `JobSchedulingExecutionEvidence` queue rows after `execute(batch)` drains. Existing foundation/topology package evidence remains unchanged and reports work stealing as zero unless the new selected smoke flag is requested.

**Tech Stack:** C++23, `MK_core`, `std::thread`, `std::mutex`, `std::condition_variable`, existing `JobSchedulingExecutionEvidence`, existing PowerShell validation entrypoints.

---

**Plan ID:** `job-execution-work-stealing-v1`

**Status:** Active.

## Official References

- Microsoft GDK task queue design: task queues expose automatic and manual dispatch modes, and manual dispatch gives game workloads explicit control over where callbacks run: https://learn.microsoft.com/en-us/gaming/gdk/docs/features/common/async/async-task-queue-design
- oneTBB scheduler design: each worker owns a deque; local work preserves locality, and stealing older work from another deque converts available parallelism into actual parallelism: https://uxlfoundation.github.io/oneTBB/main/tbb_userguide/How_Task_Scheduler_Works.html
- oneTBB scheduler guidance: NUMA, preferred core type, max threads per core, and constrained task arenas are separate policy controls with composability risks, so this slice does not claim those policies: https://www.intel.com/content/www/us/en/docs/onetbb/developer-guide-api-reference/2021-13/guiding-task-scheduler-execution.html
- oneTBB core-type selector: hybrid CPU core-type ranking is specialized policy work and remains a later host-evidence phase: https://www.intel.com/content/www/us/en/docs/onetbb/developer-guide-api-reference/2023-0/core-type-selector-for-task-arena-constraints.html

## Files

- Modify `engine/core/include/mirakana/core/job_execution.hpp` for opt-in desc/result/policy fields.
- Modify `engine/core/src/job_execution.cpp` for stealing, counters, and scheduling-evidence counter injection.
- Modify `tests/unit/core_tests.cpp` for deterministic work-stealing execution coverage.
- Modify `games/sample_desktop_runtime_game/main.cpp` for `--require-job-execution-work-stealing` evidence.
- Modify `tools/validate-installed-desktop-runtime.ps1` for installed smoke counter expectations.
- Modify `games/sample_desktop_runtime_game/game.agent.json` and `games/sample_desktop_runtime_game/README.md` for package-visible guidance.
- Modify `docs/current-capabilities.md`, `docs/roadmap.md`, `docs/superpowers/plans/README.md`, and `docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md` for plan/state sync.
- Modify `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` and compose `engine/agent/manifest.json`.
- Modify agent/static checks only if literal needles need new package counters or capability wording.

## Tasks

### Task 1: Core API And Deterministic Unit Test

- [x] Add `work_stealing_enabled` to `JobExecutionPoolDesc`.
- [x] Add `work_stealing_applied`, `steal_attempt_count`, `steal_success_count`, and `worker_wait_count` to `JobExecutionRunResult`.
- [x] Add `enable_work_stealing` to `JobExecutionTopologyPolicyDesc` and `work_stealing_applied` to `JobExecutionTopologyPolicy`.
- [x] Write a unit test that constructs a two-worker pool with `work_stealing_enabled = true`, queues two independent tasks to worker 0, blocks the first task briefly until the second starts, and asserts that worker 1 executed stolen work, steal success is positive, deterministic merge order is preserved, and no affinity/NUMA/SIMD/GPU flags are set.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_core_tests`.

### Task 2: Work-Stealing Execution And Evidence

- [x] Implement worker-local pop plus steal-from-other-workers without holding two worker queue locks at once.
- [x] Notify all workers when stealing is enabled so idle workers can compete for pending queued work.
- [x] Record attempt/success counters on the stealing worker and wait counters when workers sleep with no local or stolen work.
- [x] Inject runtime counters into `JobSchedulingExecutionEvidence.queue_rows` after `execute(batch)` drains and recompute `scheduling_summary`.
- [x] Keep default `work_stealing_enabled = false` so existing foundation evidence remains zero.
- [x] Re-run `MK_core_tests` focused build and CTest.

### Task 3: Package Smoke Evidence

- [x] Add `--require-job-execution-work-stealing` parsing and status output in `sample_desktop_runtime_game`.
- [x] Add a separate sample evidence builder that enables work stealing, queues stealable independent tasks, proves `job_execution_work_stealing_applied=1`, positive steal attempts/successes, two worker threads, deterministic merges, clean diagnostics, positive scratch rows, and zero affinity/NUMA/SIMD/GPU/CUDA/HIP/SYCL side-effect counters.
- [x] Update installed desktop runtime validation expectations for the new flag.
- [x] Update `game.agent.json` validation recipes to include the new flag on selected D3D12/Vulkan package smokes only when the package smoke already proves job execution foundation/topology.
- [x] Run the focused desktop runtime build and selected smoke/CTest lane.

### Task 4: Docs, Manifest, And Agent Surface

- [x] Update docs and plan registry so `currentActivePlan` selects this plan during execution and the backlog includes `job-execution-work-stealing-v1`.
- [x] Update current capabilities and roadmap to say work stealing is selected/implemented only after evidence, while affinity, NUMA placement, SMT/hybrid, SIMD, GPU async overlap, CUDA/HIP/SYCL, allocator enforcement, and broad optimization remain future.
- [x] Compose `engine/agent/manifest.json` from fragments.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` if agent-facing skill or rule text changes.

### Task 5: Slice Validation And Publication

- [x] Run formatting/static checks relevant to touched files.
- [x] Run full `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [ ] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1 -Branch codex/job-execution-work-stealing-v1`.
- [ ] Commit only task-owned files with validation evidence.
- [ ] Push `codex/job-execution-work-stealing-v1`.
- [ ] Create a PR with official-source notes, validation evidence, and explicit non-claims.
- [ ] Wait for required hosted checks, merge through GitHub Flow with matched head SHA, sync `main`, and remove the merged worktree with `tools/remove-merged-worktree.ps1`.

## Done When

- `JobExecutionPool` has opt-in work stealing with deterministic, drain-before-return behavior.
- Unit tests prove actual stolen execution and runtime counters.
- `sample_desktop_runtime_game` exposes selected package-visible work-stealing evidence.
- Agent-facing docs and manifest fragments accurately describe the implemented surface and remaining non-claims.
- Focused checks, full validation, PR checks, merge, and worktree cleanup complete.
