# 2026-06-03 Long-Running Performance Readiness v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` or focused inline execution only after an operator explicitly selects this plan for implementation. Steps use checkbox (`- [ ]`) syntax for tracking.

**Status:** Proposed; not selected; do not implement until the operator approves this plan and the manifest is intentionally pointed at it.

**Goal:** Make the engine able to run representative games for long sessions with stable frame pacing, bounded memory growth, deterministic diagnostics, and evidence-driven CPU/GPU optimization decisions without reopening Engine 1.0 blockers.

**Architecture:** Treat long-running readiness as an evidence and guardrail milestone before low-level tuning. The first phases add soak profiles, package-visible counters, profiling matrices, and fail-closed host gates. Later phases may execute Linux affinity, NUMA placement, broader SIMD, GPU async overlap, or CUDA/HIP/SYCL only when a previous phase proves a workload, host class, and validation lane that need the feature.

**Tech Stack:** C++23, `MK_core`, `MK_runtime`, `MK_renderer`, `MK_rhi_*`, existing package smoke commands, Trace Event JSON diagnostics, CTest, PowerShell validation wrappers, platform profilers, Linux scheduler/NUMA documentation, D3D12/Vulkan queue evidence, CUDA/HIP/SYCL official documentation for optional research lanes.

---

## Plan ID

`long-running-performance-readiness-v1`

## Current Selection State

- Current live production pointer remains `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`.
- `recommendedNextPlan.id` remains `next-production-gap-selection`.
- `unsupportedProductionGaps` remains `[]`.
- This plan must not change `currentActivePlan`, manifest fragments, static checks, or package recipes until the operator explicitly selects it for implementation.

## Official References

- Linux `sched_setaffinity(2)`: CPU affinity is a per-thread mask operation, can migrate a thread to an allowed CPU, can be restricted by cpuset/cgroups, and needs dynamic CPU-set handling on large systems.
  <https://man7.org/linux/man-pages/man2/sched_setaffinity.2.html>
- Linux NUMA memory policy: memory policy controls allocation nodes separately from cpusets, has scoped policies, and defaults to local allocation after boot.
  <https://www.kernel.org/doc/html/latest/admin-guide/mm/numa_memory_policy.html>
- Intel Intrinsics Guide: official x86 intrinsic reference covering SSE, AVX, AVX2, AVX-512, AMX, and throughput/latency metadata sourced from Intel manuals.
  <https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html>
- NVIDIA CUDA C++ Best Practices Guide: async transfer/compute overlap requires pinned memory, stream selection, `asyncEngineCount`, and measurement; concurrent kernels require device capability and non-default streams.
  <https://docs.nvidia.com/cuda/cuda-c-best-practices-guide/index.html>
- AMD HIP asynchronous concurrent execution: HIP async work runs through streams, overlap depends on device properties such as `asyncEngineCount`, and explicit synchronization/events are required.
  <https://rocm.docs.amd.com/projects/HIP/en/latest/how-to/hip_runtime_api/asynchronous.html>
- Khronos Vulkan Guide queues: work submitted to one queue starts in order but may complete out of order; work submitted to different queues is unordered unless explicitly synchronized.
  <https://docs.vulkan.org/guide/latest/queues.html>
- Microsoft Direct3D 12 multi-engine synchronization: D3D12 exposes 3D, compute, and copy queues, but applications need explicit fences for queue synchronization.
  <https://learn.microsoft.com/en-us/windows/win32/direct3d12/user-mode-heap-synchronization>
- Khronos SYCL 2020 specification: queue submission returns events, SYCL objects can block in specific host-device coordination cases, and available backends/dependencies are runtime-installation dependent.
  <https://registry.khronos.org/SYCL/specs/sycl-2020/html/sycl-2020.html>
- Context7 documentation check: CUDA Toolkit, ROCm HIP, and Khronos SYCL references were resolved and queried on 2026-06-03; their results support the same stream/event/profiling/backend-dependency constraints above.

## Existing Repo Evidence

- `docs/current-capabilities.md` states that Linux affinity, NUMA allocation execution, broad SIMD quality, GPU async overlap, CUDA/HIP/SYCL, cross-vendor/backend performance parity, and broad CPU/GPU/memory optimization remain unclaimed.
- `docs/specs/2026-06-01-engine-performance-optimization-foundation-v1.md` already orders the relevant future waves: Intel/AMD CPU Profiling Matrix v1, Parallel Command Recording v1, and Optional GPU Compute Review v1.
- Completed slices already provide memory lifetime taxonomy, memory diagnostics, frame/thread scratch, job scheduling evidence, worker pool, topology policy, work stealing, placement policy, Windows CPU Sets worker placement, SIMD dispatch, and AVX2 reviewed target execution.
- Existing package evidence intentionally reports zero affinity, NUMA, GPU async-overlap, CUDA, HIP, and SYCL side effects for the baseline lanes.

## Non-Goals

- Do not implement Linux affinity, manual NUMA placement, broad SIMD, AVX-512, NEON, CUDA, HIP, SYCL, PGO/LTO, GPU-driven rendering, or broad async compute in the first phase.
- Do not change default validation to require vendor profilers, CUDA Toolkit, ROCm, SYCL runtimes, Linux-only tools, Apple-only tools, or GPU capture tools.
- Do not expose native thread handles, affinity masks, NUMA masks, allocator handles, RHI handles, CUDA/HIP/SYCL handles, queues, fences, or backend objects through public gameplay APIs.
- Do not claim "long-running ready", "all cores used", "async overlap", "cross-vendor parity", or "broad optimization" without package-visible counters, profiler evidence, host details, and regression budgets for the exact workload.
- Do not copy vendor sample code into the repository. Use official behavior and APIs as reference only; implementation must be first-party.

## Host Classes And Evidence Fields

Every phase that makes a performance or long-running claim must record these fields in docs, package evidence, or attached artifacts:

- Workload: package/game target, scene, frame count, warmup count, replay seed, camera/input path, and selected validation recipe.
- Runtime session: session duration, frame p95/p99, max frame, dropped/over-budget frame count, queue wait counts, diagnostics count, shutdown status, and leak/stale-generation status.
- Memory: per-class current bytes, high-water bytes, allocation counts, reset counts, resident package bytes, resident GPU bytes, upload/staging bytes, transient GPU alias rows, stale-generation count, cross-thread-free count, and budget-pressure status.
- CPU host: OS, kernel/build where relevant, CPU model, sockets, physical cores, logical processors, SMT state, processor groups, NUMA node count, NPS state if available, scheduler policy, thermal/power mode, compiler, flags, and selected SIMD lane.
- GPU host: backend, GPU model, driver/runtime version, queue family/queue type, timestamp support, profiler tool/version, copy/compute capability rows, barrier/queue-wait counts, and backend-specific non-claims.
- Vendor API host: CUDA Toolkit/runtime version, ROCm/HIP version, SYCL implementation/backends, device properties, stream/event capability rows, and explicit dependency/install blocker rows.

## Phase 0: Selection And Baseline Drift Audit

**Goal:** Select this plan without accidentally claiming any optimization feature.

**Files if implemented:**

- Modify `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`.
- Compose `engine/agent/manifest.json`.
- Modify `docs/superpowers/plans/README.md`.
- Modify `docs/current-capabilities.md` only to add selected-plan context, not ready claims.
- Modify static checks only if new selected-plan literals are enforced.

- [ ] Confirm operator approval to select `long-running-performance-readiness-v1`.
- [ ] Update `currentActivePlan` and `recommendedNextPlan.id` only after approval.
- [ ] Keep `unsupportedProductionGaps = []` unless a concrete blocker is discovered and documented.
- [ ] Add a registry row that states this is a post-1.0 long-running readiness milestone, not a reopened 1.0 gap.
- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
```

Expected: manifest composition and static contract checks pass with no new ready claims for Linux affinity, NUMA execution, broad SIMD, GPU async overlap, CUDA, HIP, or SYCL.

## Phase 1: Long-Run Soak Evidence Contract

**Goal:** Define and expose a selected package smoke contract for bounded long sessions before adding low-level tuning.

**Files if implemented:**

- Modify the selected sample package executable that already owns performance budget counters.
- Modify focused unit/package tests for the new evidence rows.
- Modify installed package validation wrappers only when the new flag is selected.
- Update `docs/current-capabilities.md`, `docs/roadmap.md`, plan registry, manifest fragments, and static checks for new counters.

- [ ] Add a selected package flag named `--require-long-run-performance-readiness`.
- [ ] Emit deterministic counters for `long_run_readiness_status`, `long_run_readiness_ready`, `long_run_readiness_frames`, `long_run_readiness_warmup_frames`, `long_run_readiness_p95_frame_time_us`, `long_run_readiness_p99_frame_time_us`, `long_run_readiness_max_frame_time_us`, `long_run_readiness_over_budget_frames`, `long_run_readiness_memory_high_water_bytes`, `long_run_readiness_memory_growth_bytes`, `long_run_readiness_diagnostics`, `long_run_readiness_shutdown_clean`, and zero unsupported side-effect counters.
- [ ] Add a short default validation run that remains deterministic and cheap.
- [ ] Add a separate host-owned soak recipe that may run longer, for example 30 minutes or a fixed high frame count, but is not required in default validation.
- [ ] Make the package flag fail closed if counters are missing, non-finite, negative, or over budget.
- [ ] Preserve zero side-effect counters for Linux affinity, NUMA execution, broad SIMD, GPU async overlap, CUDA, HIP, and SYCL.

**Validation if implemented:**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target sample_desktop_runtime_game
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R sample_desktop_runtime_game
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
```

Expected: selected package counters are present, deterministic, and still make no unsupported optimization claims.

## Phase 2: Intel/AMD CPU Profiling Matrix v1

**Goal:** Decide whether Linux affinity, NUMA placement, broader SIMD, PGO/LTO, or data-layout work is justified by host evidence.

**Files if implemented:**

- Add value-row APIs in `MK_core` only if current evidence models cannot represent the required fields.
- Modify package smoke output to include host/profile references only as data, not execution side effects.
- Update docs/manifest/static checks for evidence fields.

- [ ] Define representative host classes: mainstream Intel desktop/laptop, Intel hybrid P-core/E-core, AMD Ryzen, AMD Threadripper, AMD EPYC/NPS, Linux CI host, and Windows CI host.
- [ ] Define required CPU fields: exact model, topology, SMT, processor groups, NUMA/NPS, scheduler context, compiler/flags, selected SIMD lane, thermal/power state, profiler name/version, and counters.
- [ ] Define trace recipes for CPU frame time, worker utilization, queue waits, cache behavior, branch misses, memory bandwidth, false sharing, and NUMA locality.
- [ ] Classify each finding as one of: data-layout candidate, batch-size candidate, scheduler policy candidate, Linux affinity candidate, NUMA placement candidate, SIMD kernel candidate, compiler-lane candidate, or non-goal.
- [ ] Require before/after traces and regression budgets before any follow-up implementation plan can execute CPU tuning.

**Validation if implemented:**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
```

Expected: the matrix is schema/static-check visible and all host-missing cases report host-gated evidence rather than ready claims.

## Phase 3: Linux Affinity Evidence Gate

**Goal:** Make Linux affinity a host-side execution candidate only after Phase 2 proves scheduler migration or cache-locality harm.

**Files if implemented:**

- Add private Linux platform adapter rows only when selected.
- Keep `MK_core` policy value-only.
- Update Linux package recipe documentation and static checks for side-effect counters.

- [ ] Record Linux allowed CPUs from `sched_getaffinity` or equivalent host-side adapter logic.
- [ ] Represent cpuset/cgroup restrictions explicitly, because the kernel can silently restrict the actual CPU set.
- [ ] Use dynamic CPU-set sizing for large systems; do not assume `cpu_set_t` covers every possible CPU.
- [ ] Add dry-run placement rows before any call to `sched_setaffinity`.
- [ ] Execute affinity only in an opt-in host recipe, never in default package validation.
- [ ] Emit `linux_affinity_policy_applied=1` only when the host call succeeds and the post-call observed affinity matches the requested allowed set.
- [ ] Emit failure diagnostics for missing privilege, invalid mask, cpuset restriction, unsupported host, and post-call mismatch.

**Done When:** A Linux host recipe proves one selected workload benefits or remains within regression budget with affinity applied. Until then, Linux affinity remains unclaimed.

## Phase 4: NUMA Locality And Placement Gate

**Goal:** Keep first-touch locality as the default and permit manual NUMA policy only when host evidence proves it helps.

**Files if implemented:**

- Add value-only NUMA locality rows first.
- Add platform-specific memory-policy execution only in a separate host adapter plan if needed.
- Update package evidence and docs with first-touch versus manual-placement status.

- [ ] Record NUMA node count, CPU-to-node mapping, memory policy scope, observed local/remote memory counters where available, and package workload.
- [ ] Add a first-touch locality recipe that initializes per-worker/per-chunk data on the worker expected to use it.
- [ ] Compare first-touch baseline against any proposed manual memory policy.
- [ ] Fail closed when cpuset restrictions, missing NUMA APIs, missing profiler counters, or missing NPS state make the claim unverifiable.
- [ ] Only after measured need, plan host-specific memory policy execution as its own slice.

**Done When:** NUMA placement is either rejected as unnecessary for the selected workload or selected for a host-specific follow-up with measured locality evidence.

## Phase 5: Narrow SIMD Kernel Expansion Gate

**Goal:** Continue kernel-by-kernel SIMD expansion without a broad SIMD claim.

**Files if implemented:**

- Modify only the selected hot kernel source/header/test/package rows.
- Add target-local build flags only for the selected private translation unit or object target.
- Update docs and manifest non-claims.

- [ ] Select one hot kernel from Phase 2 traces.
- [ ] Keep a scalar reference implementation and deterministic equivalence tests.
- [ ] Add runtime dispatch only for the selected lane, such as SSE2, AVX2, optional AVX-512, or future NEON.
- [ ] Keep ISA compile flags target-local; do not set global `/arch:*`, `-mavx*`, `-march=native`, or default release flags.
- [ ] For AVX-512, require host thermal/frequency and hybrid-core compatibility evidence before selection.
- [ ] For NEON, require a non-x86 host/compiler lane and scalar fallback evidence.

**Done When:** One measured kernel improves or is rejected without changing the broad engine optimization claim.

## Phase 6: GPU Async Evidence Gate

**Goal:** Prove async compute/copy overlap only on a selected backend/workload with calibrated timestamps and profiler evidence.

**Files if implemented:**

- Modify only selected renderer/RHI queue telemetry and package counters.
- Keep backend-specific proof scoped to the backend.
- Update `docs/current-capabilities.md`, manifest fragments, package validation, and static checks.

- [ ] Select one workload where D3D12 or Vulkan traces show queue waits, upload stalls, compute/graphics overlap opportunity, or CPU submit/build bottleneck.
- [ ] For D3D12, record queue types, command list counts, fence signal/wait values, queue waits, and timestamp deltas.
- [ ] For Vulkan, record queue family support, queue submission ordering, semaphore/timeline semaphore dependencies, queue ownership transfers if any, and timestamp deltas.
- [ ] Use profiler captures where available; if profiler access is missing, report host-gated evidence.
- [ ] Do not transfer D3D12 proof to Vulkan or Metal, or Vulkan proof to D3D12 or Metal.
- [ ] If CPU submit/build is the bottleneck, split into `Parallel Command Recording v1` before async-overlap execution.

**Done When:** One backend/workload has measured overlap evidence or a documented rejection, with cross-backend non-claims preserved.

## Phase 7: Optional GPU Compute Review v1

**Goal:** Decide whether CUDA, HIP, or SYCL belongs anywhere in the engine without making it a default runtime dependency.

**Files if implemented:**

- Add a review-only classifier under docs/tools or manifest evidence.
- Do not add `vcpkg.json` features, CMake dependencies, or runtime linkage in this phase.

- [ ] List each compute candidate and classify it as `rhi_compute`, `offline_tool_acceleration`, `cuda_hip_private_adapter_candidate`, `sycl_private_adapter_candidate`, or `non_goal`.
- [ ] Require evidence for data transfer cost, memory residency, synchronization, stream/event usage, queue/profiler visibility, and dependency burden.
- [ ] Prefer backend-neutral RHI compute for runtime rendering/simulation workloads.
- [ ] Permit CUDA/HIP/SYCL only for optional tooling or private adapters with clear install gates and scalar/RHI fallback paths.
- [ ] Reject candidates where vendor runtime installation, backend availability, or synchronization proof would make default validation fragile.

**Done When:** Every candidate has an explicit classification and no CUDA/HIP/SYCL runtime dependency has been introduced.

## Phase 8: Long-Run Closeout And Next Selection

**Goal:** Close the milestone without broadening claims.

- [ ] Update docs, plan registry, manifest fragments, composed manifest, schemas/static checks, and package validation expectations to match actual evidence.
- [ ] Keep remaining non-claims explicit: Linux affinity, NUMA execution, broad SIMD, GPU async, CUDA/HIP/SYCL, cross-vendor parity, cross-backend parity, and broad optimization remain unclaimed unless an exact phase proves otherwise.
- [ ] Run focused checks required by touched surfaces.
- [ ] Run full validation only when C++/runtime/build/packaging/public-contract surfaces changed.
- [ ] Run publication preflight before staging, push, PR, or merge.

**Validation if implemented at closeout:**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1
```

Expected: all selected evidence, docs, manifest, package counters, and remaining non-claims agree.

## Recommended Execution Order

1. Phase 0 only after operator selection.
2. Phase 1 long-run evidence contract.
3. Phase 2 Intel/AMD CPU Profiling Matrix v1.
4. Phase 6 GPU Async Evidence Gate if renderer traces are the current priority.
5. Phase 7 Optional GPU Compute Review v1 if vendor compute is still being considered.
6. Phase 3, Phase 4, or Phase 5 only when Phase 2 identifies a measured CPU-side need.

## Done When

- A selected package can run a deterministic long-run readiness smoke with bounded frame, memory, diagnostics, and shutdown evidence.
- Host-owned long soak recipes exist without making default validation slow or vendor-tool dependent.
- CPU profiling evidence decides whether Linux affinity, NUMA, broad SIMD, PGO/LTO, or data-layout follow-ups are justified.
- GPU async and vendor compute candidates are classified through backend-specific evidence instead of assumptions.
- Docs, manifest, static checks, and package evidence preserve exact claims and non-claims.
- `unsupportedProductionGaps` remains `[]` unless a concrete, documented production blocker is discovered.
