# Job Execution Topology Policy v1 - 2026-06-02

**Goal:** Add a portable worker-count selection policy on top of Job Execution Worker Pool v1 so engine and package evidence can choose bounded CPU worker counts from observed logical-processor evidence without claiming affinity pinning, NUMA placement, SIMD dispatch, GPU async overlap, or vendor-specific all-core optimization.

**Architecture:** Keep the policy in `MK_core` and keep it value-first. The policy observes or accepts a logical-processor count, applies fallback, reserve, and cap rules, emits diagnostics when processor-group or NUMA evidence is required, and returns a `JobExecutionPoolDesc` plus topology rows for the existing `JobExecutionPool`. It does not call OS affinity APIs, expose native thread handles, apply NUMA placement, or select SIMD/GPU paths.

**Tech Stack:** C++23 `MK_core`, `std::thread::hardware_concurrency`, `JobExecutionPoolDesc`, `JobWorkerTopologyRow`, `ScratchArena`, selected `sample_desktop_runtime_game` package counters, PowerShell 7 validation wrappers, composed `engine/agent/manifest.json`, and static checks under `tools/check-ai-integration*.ps1` / `tools/check-json-contracts*.ps1`.

---

**Plan ID:** `job-execution-topology-policy-v1`

**Status:** Completed.

Selected on 2026-06-02 after [Job Execution Worker Pool v1](2026-06-02-job-execution-worker-pool-v1.md) added real bounded worker-thread execution and package-visible `job_execution_foundation_*` evidence.

Completed through PR #382 / merge commit `f277e990dfc27afea4492c7a1990969fd47a7c39`. The closeout returned `currentActivePlan` to the production-completion master selection gate with `recommendedNextPlan.id = next-production-gap-selection` and `unsupportedProductionGaps = []`.

## Official Guidance

- Microsoft C++ documentation describes `std::thread::hardware_concurrency()` as an estimate of hardware thread contexts and documents that it may return zero. The policy must therefore accept explicit observed counts, retain a fallback path, and report whether fallback was used.
- Microsoft C++ threading guidance treats `std::thread` as the standard thread primitive and keeps thread lifetime explicit. The worker pool keeps RAII-owned workers; this policy only selects the count and derived pool descriptor.
- Windows processor-group guidance matters for high logical-processor counts. This slice records `host_evidence_required` when multiple processor groups are declared without explicit processor-group evidence instead of inventing portable affinity behavior.
- NUMA and vendor CPU topology guidance from Intel and AMD remains a follow-up input. This slice records when NUMA topology evidence is missing and does not place workers or memory by node.
- CUDA, HIP, SYCL, RDNA, and Intel GPU execution paths remain outside this CPU topology policy. GPU async overlap needs renderer/RHI and backend-specific plans with package or host evidence.

## Constraints

- Keep `engine/core` independent from OS APIs, GPU APIs, editor code, renderer/RHI, and asset formats.
- Do not infer "use all cores" from `hardware_concurrency`; require reserve/cap/request rules and deterministic diagnostics.
- Do not add work stealing, affinity masks, NUMA placement controls, SMT/hybrid CPU policy, SIMD dispatch, GPU async overlap, CUDA/HIP/SYCL, or vendor-specific policy in this slice.
- Keep package-visible counters deterministic; use fixed sample evidence for installed package validation rather than host-variable all-core counts.
- Fail closed on invalid effective logical processors, zero queue capacity, or zero scratch budget.

## Phase 0 - Selection And Contract

**Files:**
- Create: `docs/superpowers/plans/2026-06-02-job-execution-topology-policy-v1.md`
- Modify: `docs/superpowers/plans/2026-06-02-job-execution-worker-pool-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/specs/2026-06-01-engine-performance-optimization-foundation-v1.md`
- Modify: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `engine/agent/manifest.fragments/014-gameCodeGuidance.json`
- Modify: `engine/agent/manifest.fragments/015-aiDrivenGameWorkflow.json`
- Modify: static checks for active-plan and package-counter literals

- [x] Select this plan as `currentActivePlan` and `recommendedNextPlan.id = job-execution-topology-policy-v1`.
- [x] Mark Job Execution Worker Pool v1 as completed evidence and keep `unsupportedProductionGaps = []`.
- [x] Record official guidance and explicit non-claims.

## Phase 1 - Core Topology Policy

**Files:**
- Modify: `engine/core/include/mirakana/core/job_execution.hpp`
- Modify: `engine/core/src/job_execution.cpp`
- Modify: `tests/unit/core_tests.cpp`

- [x] Add `JobExecutionTopologyPolicyDesc` with observed logical processors, fallback count, requested worker count, cap, reserve count, queue capacity, scratch budget, frame index, processor-group count, NUMA node count, and evidence flags.
- [x] Add `JobExecutionTopologyPolicy` with selected worker count, fallback/cap/clamp/request flags, derived `JobExecutionPoolDesc`, topology row, status, diagnostics, and explicit zero side-effect flags for affinity, NUMA, SIMD, and GPU overlap.
- [x] Add `select_job_execution_topology_policy`, `observe_job_execution_logical_processor_count`, and label helpers.
- [x] Fail closed for invalid configuration and report `host_evidence_required` for missing processor-group or NUMA evidence.
- [x] Keep the worker pool itself unchanged except for consuming the derived descriptor in callers.

**Phase 1 Focused Validation:**

- TDD red: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests` failed before the topology-policy API existed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests`: pass.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_core_tests`: pass.

## Phase 2 - Package Evidence Mapping

**Files:**
- Modify: `games/CMakeLists.txt`
- Modify: `games/sample_desktop_runtime_game/main.cpp`
- Modify: `games/sample_desktop_runtime_game/game.agent.json`
- Modify: `tools/validate-installed-desktop-runtime.ps1`
- Modify: `games/sample_desktop_runtime_game/README.md`

- [x] Add `--require-job-execution-topology-policy` to selected source-tree and installed sample smokes.
- [x] Emit package-visible `job_execution_topology_policy_*` counters with deterministic sample evidence.
- [x] Reuse the selected policy-derived `JobExecutionPoolDesc` for the sample worker-pool package evidence path.
- [x] Assert installed package counters in `tools/validate-installed-desktop-runtime.ps1`.
- [x] Preserve zero side-effect counters for processor-group policy, NUMA policy, affinity, SIMD, GPU async overlap, CUDA, HIP, and SYCL.

**Phase 2 Focused Validation:**

- TDD red: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset desktop-runtime --output-on-failure -R sample_desktop_runtime_game_smoke` failed on unknown `--require-job-execution-topology-policy` before implementation.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset desktop-runtime`: pass.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset desktop-runtime --target sample_desktop_runtime_game`: pass.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset desktop-runtime --output-on-failure -R sample_desktop_runtime_game_smoke`: pass.
- Direct smoke output from `out\build\desktop-runtime\games\Debug\sample_desktop_runtime_game\sample_desktop_runtime_game.exe --smoke --require-job-scheduling-evidence --require-job-execution-foundation --require-job-execution-topology-policy` reported `job_execution_topology_policy_status=ready`, `job_execution_topology_policy_ready=1`, `job_execution_topology_policy_observed_logical_processors=8`, `job_execution_topology_policy_effective_logical_processors=8`, `job_execution_topology_policy_selected_worker_count=2`, `job_execution_topology_policy_worker_count_limit=2`, `job_execution_topology_policy_reserved_logical_processors=1`, `job_execution_topology_policy_worker_count_limited_by_cap=1`, and zero processor-group/NUMA/affinity/SIMD/GPU-overlap/CUDA/HIP/SYCL side-effect counters.

## Validation Evidence

Close the slice with focused checks first, then full repository validation:

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_core_tests`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset desktop-runtime --target sample_desktop_runtime_game`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset desktop-runtime --output-on-failure -R sample_desktop_runtime_game_smoke`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Preset dev -Configuration Debug -ReuseExistingFileApiReply -Files engine/core/src/job_execution.cpp,tests/unit/core_tests.cpp`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Preset desktop-runtime -Configuration Debug -ReuseExistingFileApiReply -Files games/sample_desktop_runtime_game/main.cpp`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`

## Done When

- `MK_core` exposes a portable topology policy that selects bounded worker counts, uses explicit fallback/cap/reserve rules, and derives `JobExecutionPoolDesc` without OS/backend dependencies.
- Tests prove normal selection, cap behavior, fallback behavior, and host-evidence-required diagnostics for processor-group and NUMA evidence gaps.
- Selected package smokes and installed package validation prove `job_execution_topology_policy_*` counters.
- Agent/docs/manifest/static checks distinguish portable worker-count policy from still-unclaimed affinity, NUMA placement, SMT/hybrid CPU policy, SIMD dispatch, GPU async overlap, CUDA/HIP/SYCL, cross-vendor/backend parity, and broad all-core CPU/GPU/memory optimization readiness.
- Relevant focused checks and full validation pass, or concrete host/tool blockers are recorded.
