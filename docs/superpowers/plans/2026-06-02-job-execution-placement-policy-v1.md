# Job Execution Placement Policy v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a first-party, host-independent CPU placement policy evidence layer for `MK_core` job execution so AI/game/host workflows can reason about affinity, NUMA, SMT, and hybrid-core choices before any OS-specific pinning or vendor-specific optimization claim.

**Architecture:** Keep OS scheduling as the default execution path and make placement policy an explicit evidence contract. `MK_core` will derive deterministic placement rows from observed topology, requested policy, NUMA/processor-group evidence, SMT/hybrid hints, and completed worker-pool/work-stealing settings, but it will not call Windows CPU Set APIs, Linux affinity APIs, or vendor runtimes. Host adapters may later execute affinity or NUMA placement only through separate host-gated plans with measured evidence.

**Tech Stack:** C++23, `MK_core`, existing `JobExecutionTopologyPolicy`, existing `JobSchedulingExecutionEvidence`, existing `sample_desktop_runtime_game` package evidence, PowerShell validation entrypoints.

---

**Plan ID:** `job-execution-placement-policy-v1`

**Status:** Active.

## Official References

- Microsoft Windows processor groups: Windows 11 and Windows Server 2022 default process/thread affinities can span all processor groups, so engine policy must not assume manual group pinning is required on modern Windows: https://learn.microsoft.com/en-us/windows/win32/procthread/processor-groups
- Microsoft Windows CPU Sets: CPU Set assignment is an advanced optional scheduling constraint and interacts with group affinity; host-side application belongs behind explicit evidence: https://learn.microsoft.com/en-us/windows/win32/procthread/cpu-sets
- Microsoft `SetThreadSelectedCpuSetMasks`: group-affinity-based CPU Set selection is the Windows host API candidate for later execution, not a dependency-free `MK_core` side effect: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-setthreadselectedcpusetmasks
- Microsoft NUMA processor masks: `GetNumaNodeProcessorMaskEx` is required for processor masks across groups, so NUMA execution needs host evidence rather than inferred single-mask assumptions: https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getnumanodeprocessormask
- Linux `sched_setaffinity(2)`: Linux affinity is a thread/process mask operation and remains host-side execution, not a core-module policy side effect: https://man7.org/linux/man-pages/man2/sched_setaffinity.2.html
- Linux `numa(3)`: NUMA memory policy and node-local allocation are separate from generic worker-count selection and require measured locality evidence: https://man7.org/linux/man-pages/man3/numa.3.html
- Intel oneTBB core-type selector: official guidance says hybrid CPU systems generally should allow OS scheduling across core types; explicit core-type selection is an advanced constraint with feature gates: https://www.intel.com/content/www/us/en/docs/onetbb/developer-guide-api-reference/2023-0/core-type-selector-for-task-arena-constraints.html
- Intel optimization reference: CPU optimization should be evidence-driven and architecture-aware rather than a broad all-Intel ready claim: https://www.intel.com/content/www/us/en/developer/articles/technical/intel64-and-ia32-architectures-optimization.html
- AMD Zen Software Studio: AMD positions AOCC/AOCL/uProf as processor-optimized tooling, so AMD-specific promotion needs a later compiler/library/profiling evidence lane rather than generic core claims: https://www.amd.com/en/developer/zen-software-studio.html

## Files

- Modify `engine/core/include/mirakana/core/job_execution.hpp` for placement policy enums, descriptors, result rows, diagnostics, and label helpers.
- Modify `engine/core/src/job_execution.cpp` for deterministic policy selection over completed topology/work-stealing inputs.
- Modify `tests/unit/core_tests.cpp` for policy-ready, host-evidence-required, invalid-request, SMT/hybrid, and NUMA/processor-group diagnostics.
- Modify `games/sample_desktop_runtime_game/main.cpp` for `--require-job-execution-placement-policy` package-visible counters.
- Modify `tools/validate-installed-desktop-runtime.ps1` for installed smoke expectations if the new flag is selected.
- Modify `games/sample_desktop_runtime_game/game.agent.json` and `games/sample_desktop_runtime_game/README.md` for package-visible guidance.
- Modify `docs/current-capabilities.md`, `docs/roadmap.md`, `docs/superpowers/plans/README.md`, and `docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md` for plan/state sync.
- Modify `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` and compose `engine/agent/manifest.json`.
- Modify agent/static checks only if new literal counters or capability wording need enforced needles.

## Tasks

### Task 1: Core Policy API And Tests

- [ ] Add `JobExecutionPlacementPolicyMode` with `os_default`, `prefer_local_numa`, `prefer_performance_cores`, `prefer_efficiency_cores`, `avoid_smt_siblings`, and `manual_host_affinity` values.
- [ ] Add `JobExecutionPlacementPolicyStatus` with `ready`, `invalid_configuration`, and `host_evidence_required`.
- [ ] Add `JobExecutionPlacementPolicyDiagnosticCode` with `none`, `invalid_configuration`, `missing_processor_group_evidence`, `missing_numa_evidence`, `missing_hybrid_core_evidence`, `missing_smt_evidence`, and `host_execution_required`.
- [ ] Add `JobExecutionPlacementPolicyDesc` that consumes a completed `JobExecutionTopologyPolicy`, requested placement mode, optional NUMA node count, optional core-type class counts, optional SMT sibling evidence, and `allow_host_affinity_execution`.
- [ ] Add `JobExecutionPlacementPolicy` with deterministic row counts, selected mode, inherited `JobExecutionPoolDesc`, `affinity_policy_applied=false`, `numa_policy_applied=false`, `simd_dispatch_applied=false`, `gpu_async_overlap_applied=false`, diagnostics, and label helpers.
- [ ] Write failing tests in `tests/unit/core_tests.cpp` for OS-default ready policy, NUMA request missing NUMA evidence, hybrid-core request missing core-type evidence, manual host-affinity request requiring host execution, and invalid empty topology policy.
- [ ] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests`.
- [ ] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_core_tests`.

### Task 2: Deterministic Placement Policy Selection

- [ ] Implement `select_job_execution_placement_policy(const JobExecutionPlacementPolicyDesc& desc)` in `engine/core/src/job_execution.cpp`.
- [ ] Make `os_default` ready when the inherited topology policy is ready and no missing host-evidence diagnostics are present.
- [ ] Make NUMA, hybrid-core, SMT, and manual host-affinity modes fail closed to `host_evidence_required` until the required evidence flags are present.
- [ ] Preserve completed worker-count, queue-capacity, scratch, and work-stealing decisions in the returned `pool_desc`.
- [ ] Keep all OS API execution flags false in `MK_core`; host adapters will own any later `SetThreadSelectedCpuSetMasks`, `sched_setaffinity`, or NUMA allocation execution.
- [ ] Re-run the focused `MK_core_tests` build and CTest.

### Task 3: Package Smoke Evidence

- [ ] Add `--require-job-execution-placement-policy` parsing in `sample_desktop_runtime_game`.
- [ ] Build package evidence from the completed topology policy plus work-stealing policy, request `os_default`, and emit `job_execution_placement_policy_status=ready`, `job_execution_placement_policy_ready=1`, selected mode, inherited worker count, inherited work-stealing flag, clean diagnostics, and zero OS-affinity/NUMA/SIMD/GPU/CUDA/HIP/SYCL execution counters.
- [ ] Add a host-evidence-required diagnostic counter for a selected NUMA request with intentionally missing NUMA placement evidence, without failing the ready OS-default package lane.
- [ ] Update installed desktop runtime validation expectations for the selected flag.
- [ ] Run the focused desktop runtime build and selected package smoke lane.

### Task 4: Docs, Manifest, And Agent Surface

- [ ] Update current capabilities and roadmap to state that placement policy evidence is selected, while actual affinity pinning, NUMA placement execution, hybrid P-core/E-core pinning, SMT scheduling, SIMD dispatch, GPU async overlap, CUDA/HIP/SYCL paths, and broad optimization remain unclaimed.
- [ ] Update the production backlog row for `job-execution-placement-policy-v1` from active to implemented only after code/package evidence lands.
- [ ] Compose `engine/agent/manifest.json` from fragments with `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`.
- [ ] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [ ] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [ ] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` if agent-facing skill or rule text changes.

### Task 5: Slice Validation And Publication

- [ ] Run formatting/static checks relevant to touched files.
- [ ] Run full `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [ ] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1 -Branch codex/job-execution-placement-policy-v1`.
- [ ] Commit only task-owned files with validation evidence.
- [ ] Push `codex/job-execution-placement-policy-v1`.
- [ ] Create a PR with official-source notes, validation evidence, and explicit non-claims.
- [ ] Wait for required hosted checks, merge through GitHub Flow with matched head SHA, sync `main`, and remove the merged worktree with `tools/remove-merged-worktree.ps1`.

## Done When

- `MK_core` exposes a deterministic placement policy evidence contract over completed topology/work-stealing decisions.
- Focused tests prove OS-default readiness and fail-closed NUMA/hybrid/SMT/manual-affinity evidence requirements.
- `sample_desktop_runtime_game` exposes selected package-visible placement policy evidence without OS affinity side effects.
- Docs and manifest fragments accurately describe the implemented placement-policy surface and remaining non-claims.
- Focused checks, full validation, PR checks, merge, and worktree cleanup complete.
