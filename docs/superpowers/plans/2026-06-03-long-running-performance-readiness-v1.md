# 2026-06-03 Long-Running Performance Readiness v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` or focused inline execution only after an operator explicitly selects this plan for implementation. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `long-running-performance-readiness-v1`

**Status:** Completed.

Phase 0/1 is implemented as the first review boundary and published through PR #403, which is merged. Phase 2 and Phase 7 are implemented through PR #406 as docs/manifest/schema/static-contract slices. Phase 2 exposes the host-gated CPU profiling matrix; Phase 7 classifies optional GPU compute candidates. The milestone is closed back to `next-production-gap-selection` without implementing Linux affinity, NUMA execution, broad SIMD, PGO/LTO, GPU async, CUDA, HIP, SYCL, RHI compute changes, `vcpkg.json` features, CMake linkage, or default validation dependencies.

**Goal:** Make the engine able to run representative games for long sessions with stable frame pacing, bounded memory growth, deterministic diagnostics, and evidence-driven CPU/GPU optimization decisions without reopening Engine 1.0 blockers.

**Architecture:** Treat long-running readiness as an evidence and guardrail milestone before low-level tuning. The first phases add soak profiles, package-visible counters, profiling matrices, and fail-closed host gates. Later phases may execute Linux affinity, NUMA placement, broader SIMD, GPU async overlap, or CUDA/HIP/SYCL only when a previous phase proves a workload, host class, and validation lane that need the feature.

**Tech Stack:** C++23, `MK_core`, `MK_runtime`, `MK_renderer`, `MK_rhi_*`, existing package smoke commands, Trace Event JSON diagnostics, CTest, PowerShell validation wrappers, platform profilers, Linux scheduler/NUMA documentation, D3D12/Vulkan queue evidence, CUDA/HIP/SYCL official documentation for optional research lanes.

---

## Milestone ID

`long-running-performance-readiness-v1`

## Current Selection State

- Current live production pointer has returned to `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`.
- `recommendedNextPlan.id` is `next-production-gap-selection`.
- `unsupportedProductionGaps` remains `[]`.
- Phase 2 exposes `aiOperableProductionLoop.cpuProfilingMatrix` and `host-cpu-profiling-matrix` as the host-gated CPU profiling matrix contract.
- Phase 7 updated docs, schemas, manifest fragments, validation recipe descriptors, static checks, and composed manifest output only for `Optional GPU Compute Review v1`, `optionalGpuComputeReview`, and `host-optional-gpu-compute-review`.
- Phase 7 does not make CUDA/HIP/SYCL, RHI compute, GPU async overlap, broad GPU compute, cross-vendor parity, cross-backend parity, or broad CPU/GPU/memory optimization ready.

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
- Windows Performance Recorder: WPR records ETW events for later WPA analysis and is part of the Windows Performance Toolkit / ADK.
  <https://learn.microsoft.com/en-us/windows-hardware/test/wpt/windows-performance-recorder>
- Windows WPR PMU events: WPR/Xperf PMU counter availability is host-specific and supports enumerating available PMU sources.
  <https://learn.microsoft.com/en-us/windows-hardware/test/wpt/recording-pmu-events>
- Linux `perf stat` and `perf record`: `perf stat` gathers performance counter statistics and `perf record` records sampled profiles for later analysis.
  <https://man7.org/linux/man-pages/man1/perf-stat.1.html>
  <https://man7.org/linux/man-pages/man1/perf-record.1.html>
- Intel VTune Profiler CLI and Hotspots: VTune supports command-line collection/reporting/comparison and CPU Hotspots analysis.
  <https://www.intel.com/content/www/us/en/docs/vtune-profiler/user-guide/2024-0/command-line-interface.html>
  <https://www.intel.com/content/www/us/en/docs/vtune-profiler/user-guide/current/basic-hotspots-analysis.html>
- AMD uProf Hotspots: AMD uProf supports Hotspots and CLI-oriented host profiling workflows for CPU time and hardware counter evidence.
  <https://docs.amd.com/r/en-US/68658-uProf-getting-started-guide/AMD-uProf-Hotspots-Analysis>
- Context7 documentation check: CUDA Toolkit, ROCm HIP, and Khronos SYCL references were resolved and queried on 2026-06-03; their results support the same stream/event/profiling/backend-dependency constraints above.

## Existing Repo Evidence

- `docs/current-capabilities.md` states that Linux affinity, NUMA allocation execution, broad SIMD quality, GPU async overlap, CUDA/HIP/SYCL, cross-vendor/backend performance parity, and broad CPU/GPU/memory optimization remain unclaimed.
- `docs/specs/2026-06-01-engine-performance-optimization-foundation-v1.md` already orders the relevant future waves: Intel/AMD CPU Profiling Matrix v1, Parallel Command Recording v1, and Optional GPU Compute Review v1.
- Completed slices already provide memory lifetime taxonomy, memory diagnostics, frame/thread scratch, job scheduling evidence, worker pool, topology policy, work stealing, placement policy, Windows CPU Sets worker placement, SIMD dispatch, and AVX2 reviewed target execution.
- Existing package evidence intentionally reports zero affinity, NUMA, GPU async-overlap, CUDA, HIP, and SYCL side effects for the baseline lanes.

## Resolved Planning Decisions

- Phase 1 primary target is `games/sample_2d_desktop_runtime_package`, because it already owns the selected performance-budget lane, `--require-performance-baseline`, `performance_baseline_*` counters, and `installed-2d-performance-baseline-smoke`.
- `games/sample_desktop_runtime_game` is not the Phase 1 primary target. It remains the follow-up host/backend optimization target for CPU placement, SIMD, renderer queue evidence, D3D12/Vulkan proof, and any later GPU async evidence.
- Phase 0 and Phase 1 should be reviewed in one PR after operator approval: Phase 0 selects the plan, and Phase 1 immediately adds the first package-visible evidence contract. Do not merge a selection-only PR that leaves the active plan without fresh evidence.
- The first implementation PR must not add Linux affinity execution, NUMA policy execution, broad SIMD expansion, GPU async overlap execution, CUDA, HIP, SYCL, PGO/LTO, or any new default validation dependency.
- Longer soak execution is host-owned and opt-in. Default validation gets only a deterministic short smoke; a 30-minute or high-frame-count soak is a separate recipe with explicit host evidence.
- `unsupportedProductionGaps` remains `[]` unless a concrete blocker is discovered. Missing optional host evidence is recorded as `host-gated`, not as an Engine 1.0 blocker.

## Non-Goals

- Do not implement Linux affinity, manual NUMA placement, broad SIMD, AVX-512, NEON, CUDA, HIP, SYCL, PGO/LTO, GPU-driven rendering, or broad async compute in this contract phase.
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

- [x] Confirm operator approval to select `long-running-performance-readiness-v1`.
- [x] Update `currentActivePlan` and `recommendedNextPlan.id` only after approval.
- [x] Keep `unsupportedProductionGaps = []` unless a concrete blocker is discovered and documented.
- [x] Add a registry row that states this is a post-1.0 long-running readiness milestone, not a reopened 1.0 gap.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
```

Expected: manifest composition and static contract checks pass with no new ready claims for Linux affinity, NUMA execution, broad SIMD, GPU async overlap, CUDA, HIP, or SYCL.

## Phase 1: Long-Run Soak Evidence Contract

**Goal:** Define and expose a selected package smoke contract for bounded long sessions before adding low-level tuning.

**Files to modify during implementation:**

- Modify `games/sample_2d_desktop_runtime_package/main.cpp`.
- Modify `tools/validate-installed-desktop-runtime.ps1`.
- Modify `games/sample_2d_desktop_runtime_package/game.agent.json`.
- Modify `engine/agent/manifest.fragments/009-validationRecipes.json` only if the new recipe is promoted to the shared manifest recipe list.
- Modify `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` and `engine/agent/manifest.fragments/014-gameCodeGuidance.json`.
- Compose `engine/agent/manifest.json`.
- Modify `docs/current-capabilities.md`, `docs/roadmap.md`, `docs/ai-game-development.md`, and `docs/superpowers/plans/README.md`.
- Modify static checks in `tools/check-ai-integration-*.ps1` or `tools/check-json-contracts-*.ps1` only for literals that must remain machine-enforced.

- [x] Add a selected package flag named `--require-long-run-performance-readiness`.
- [x] Make `--require-long-run-performance-readiness` imply the same selected-package dependencies as `--require-performance-baseline`: `--require-sandbox-package-budgets`, `--require-win32-runtime-host`, `--require-win32-d3d12-presentation`, `--require-d3d12-shaders`, and `--require-d3d12-renderer`.
- [x] Emit deterministic counters for `long_run_readiness_status=ready`, `long_run_readiness_ready=1`, `long_run_readiness_frames=<max_frames>`, `long_run_readiness_warmup_frames=0`, `long_run_readiness_p95_frame_time_us`, `long_run_readiness_p99_frame_time_us`, `long_run_readiness_max_frame_time_us`, `long_run_readiness_over_budget_frames=0`, `long_run_readiness_memory_high_water_bytes>0`, `long_run_readiness_memory_growth_bytes=0`, `long_run_readiness_diagnostics=0`, `long_run_readiness_shutdown_clean=1`, and zero unsupported side-effect counters.
- [x] Use the existing deterministic baseline frame samples for the short smoke so p95/p99/max values stay reproducible and cheap.
- [x] Add installed validation assertions for every `long_run_readiness_*` counter when `SmokeArgs` contains `--require-long-run-performance-readiness`.
- [x] Add `installed-2d-long-run-readiness-smoke` to `games/sample_2d_desktop_runtime_package/game.agent.json` with the exact smoke args above.
- [x] Add a short default validation run that remains deterministic and cheap.
- [x] Add a separate host-owned soak recipe name, `host-2d-long-run-readiness-soak`, that may run longer, for example 30 minutes or a fixed high frame count, but is not required in default validation.
- [x] Make the package flag fail closed if counters are missing, non-finite, negative, or over budget.
- [x] Preserve zero side-effect counters for Linux affinity, NUMA execution, broad SIMD expansion, GPU async overlap, CUDA, HIP, SYCL, native handle exposure, and cross-backend parity.
- [x] Do not touch `games/sample_desktop_runtime_game` in Phase 1. Add a separate follow-up phase only after the 2D package lane is green.

**Required Phase 1 status-line fields:**

```text
long_run_readiness_status
long_run_readiness_ready
long_run_readiness_frames
long_run_readiness_warmup_frames
long_run_readiness_p95_frame_time_us
long_run_readiness_p99_frame_time_us
long_run_readiness_max_frame_time_us
long_run_readiness_over_budget_frames
long_run_readiness_memory_high_water_bytes
long_run_readiness_memory_growth_bytes
long_run_readiness_diagnostics
long_run_readiness_shutdown_clean
long_run_readiness_linux_affinity_applied
long_run_readiness_numa_policy_applied
long_run_readiness_broad_simd_applied
long_run_readiness_gpu_async_overlap_applied
long_run_readiness_cuda_path_used
long_run_readiness_hip_path_used
long_run_readiness_sycl_path_used
long_run_readiness_native_handles_exposed
```

**Validation if implemented:**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target sample_2d_desktop_runtime_package
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_2d_desktop_runtime_package
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Expected: selected 2D package counters are present, deterministic, and still make no unsupported optimization claims. Full validation is required before publication because Phase 1 changes C++ runtime/package/public contract surfaces.

**Phase 1 validation evidence captured before full publication validation:**

| Date | Command | Result |
| --- | --- | --- |
| 2026-06-03 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Pass: `agent-config-check: ok`. |
| 2026-06-03 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Pass: `agent-manifest-compose: ok`, `json-contract-check: ok`. |
| 2026-06-03 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Pass: `ai-integration-check: ok`. |
| 2026-06-03 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Pass: `text-format-check: ok`, `format-check: ok`. |
| 2026-06-03 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode DryRun -Recipe installed-2d-long-run-readiness-smoke -GameTarget sample_2d_desktop_runtime_package -StrictBackend D3D12` | Pass: command plan includes `--require-long-run-performance-readiness`. |
| 2026-06-03 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode DryRun -Recipe host-2d-long-run-readiness-soak -GameTarget sample_2d_desktop_runtime_package -StrictBackend D3D12` | Pass: command plan includes `--max-frames 108000`, `--require-long-run-performance-readiness`, and host gates `d3d12-windows-primary`, `long-run-host-soak`. |
| 2026-06-03 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe installed-2d-long-run-readiness-smoke -GameTarget sample_2d_desktop_runtime_package -StrictBackend D3D12 -HostGateAcknowledgements d3d12-windows-primary -TimeoutSeconds 300` | Pass: `status: passed`, `exitCode: 0`. |
| 2026-06-03 | `.\bin\sample_2d_desktop_runtime_package.exe --smoke --require-config runtime/sample_2d_desktop_runtime_package.config --require-scene-package runtime/sample_2d_desktop_runtime_package.geindex --require-win32-runtime-host --require-win32-d3d12-presentation --require-d3d12-shaders --require-d3d12-renderer --require-sandbox-package-budgets --require-performance-baseline --require-long-run-performance-readiness` from `out/install/desktop-runtime-release` | Pass: `long_run_readiness_status=ready`, `long_run_readiness_ready=1`, `long_run_readiness_frames=3`, p95/p99/max frame time `16000`, `long_run_readiness_over_budget_frames=0`, `long_run_readiness_memory_high_water_bytes=7408`, `long_run_readiness_memory_growth_bytes=0`, `long_run_readiness_diagnostics=0`, `long_run_readiness_shutdown_clean=1`, and every unsupported side-effect counter remains `0`. |

## Phase 2: Intel/AMD CPU Profiling Matrix v1

**Goal:** Decide whether Linux affinity, NUMA placement, broader SIMD, PGO/LTO, or data-layout work is justified by host evidence.

**Files if implemented:**

- Do not add `MK_core` value-row APIs in Phase 2; existing diagnostics/trace/profiler evidence models can represent this matrix as host-owned data.
- Do not modify package smoke output in Phase 2; package-visible host/profile references remain a later implementation only if the matrix proves a need.
- Update `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`, `014-gameCodeGuidance.json`, `009-validationRecipes.json`, `schemas/engine-agent/ai-operable-production-loop.schema.json`, and static checks so `cpuProfilingMatrix` is schema/static-check visible.
- Update docs/capability summaries so AVX2 is not presented as the latest CPU optimization evidence boundary after Phase 2 selection.

- [x] Define representative host classes: mainstream Intel desktop/laptop, Intel hybrid P-core/E-core, AMD Ryzen, AMD Threadripper, AMD EPYC/NPS, Linux CI host, and Windows CI host.
- [x] Define required CPU fields: exact model, topology, SMT, processor groups, NUMA/NPS, scheduler context, compiler/flags, selected SIMD lane, thermal/power state, profiler name/version, and counters.
- [x] Define trace recipes for CPU frame time, worker utilization, queue waits, cache behavior, branch misses, memory bandwidth, false sharing, and NUMA locality.
- [x] Classify each finding as one of: data-layout candidate, batch-size candidate, scheduler policy candidate, Linux affinity candidate, NUMA placement candidate, SIMD kernel candidate, compiler-lane candidate, or non-goal.
- [x] Require before/after traces and regression budgets before any follow-up implementation plan can execute CPU tuning.
- [x] Add `cpuProfilingMatrix` as a manifest/schema/static-check-visible host-gated contract.
- [x] Add `host-cpu-profiling-matrix` as a manifest validation recipe descriptor for external official-profiler artifacts, without adding default validation or runner execution.
- [x] Keep every host-missing case `host-gated`; do not mark Linux affinity, NUMA execution, broad SIMD, PGO/LTO, data-layout work, GPU async, CUDA/HIP/SYCL, cross-vendor/backend parity, or broad CPU/GPU/memory optimization ready.

**Done When:** The composed manifest exposes `aiOperableProductionLoop.cpuProfilingMatrix`, schema validation requires it, static checks assert its host classes/fields/trace recipes/classification/before-after/regression-budget/non-goals, `currentCpuProfilingMatrix` guidance is present, `host-cpu-profiling-matrix` exists as host-owned external artifact evidence, and focused checks pass.

**Validation if implemented:**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
```

Expected: the matrix is schema/static-check visible and all host-missing cases report host-gated evidence rather than ready claims.

**Phase 2 validation evidence:**

| Date | Command | Result |
| --- | --- | --- |
| 2026-06-03 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` | Pass: composed `engine/agent/manifest.json`. |
| 2026-06-03 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Pass: `agent-config-check: ok`. |
| 2026-06-03 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Pass: `agent-manifest-compose: ok`, `json-contract-check: ok`. |
| 2026-06-03 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Pass: `text-format-check: ok`, `format-check: ok`. |
| 2026-06-03 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Pass: `ai-integration-check: ok`. |
| 2026-06-03 | `git diff --check` | Pass: no whitespace errors. |

Full `tools/validate.ps1` is not required for this Phase 2 closeout because the slice changes docs, schema, manifest fragments, composed manifest output, validation recipe descriptors, and static guards only; it does not change C++ runtime, build, packaging, or public API behavior.

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

- [x] Record NUMA node count, CPU-to-node mapping, memory policy scope, observed local/remote memory counters where available, and package workload.
- [x] Add a first-touch locality recipe that initializes per-worker/per-chunk data on the worker expected to use it.
- [x] Compare first-touch baseline against any proposed manual memory policy.
- [x] Fail closed when cpuset restrictions, missing NUMA APIs, missing profiler counters, or missing NPS state make the claim unverifiable.
- [ ] Only after measured need, plan host-specific memory policy execution as its own slice.

**Done When:** NUMA placement is either rejected as unnecessary for the selected workload or selected for a host-specific follow-up with measured locality evidence.

**Phase 4 closeout decision:** For `games/sample_2d_desktop_runtime_package` on the reviewed single-node Windows D3D12 short-soak lane, value-only evidence and the first-touch versus manual memory-policy comparison reject manual NUMA placement. The package keeps `long_run_readiness_numa_memory_policy_recommendation=keep_first_touch_default`, `long_run_readiness_numa_manual_policy_locality_gain_per_mille=0`, and `long_run_readiness_numa_policy_applied=0`. Host-specific memory-policy execution remains a separate future slice only if a later host/workload proves measured locality gain.

**Phase 4 validation evidence:**

| Date | Command / PR | Result |
| --- | --- | --- |
| 2026-06-08 | PR #545 | Merged: value-only NUMA locality evidence rows and first-touch locality recipe with package counters. |
| 2026-06-08 | PR #546 | Merged: first-touch versus manual memory-policy comparison with fail-closed diagnostics and package comparison counters. |
| 2026-06-08 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Pass: `text-format-check: ok`, `format-check: ok`. |
| 2026-06-08 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests` + run | Pass: NUMA locality and memory-policy comparison unit tests green. |
| 2026-06-08 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass: static guards ok; CTest 109/109 passed. |
| 2026-06-08 | CI on PR #545 and PR #546 | Pass: Agent Static Guards, Full Repository Static Analysis (0-3), Windows MSVC, Linux, macOS, iOS. |

Full `tools/validate.ps1` is required before publication because Phase 4 changes C++ runtime/package/public contract surfaces. The installed long-run readiness smoke lane is host-gated to the packaged 2D desktop runtime and is exercised in CI Windows MSVC validation rather than in every local agent session.

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

- [x] List each compute candidate and classify it as `rhi_compute`, `offline_tool_acceleration`, `cuda_hip_private_adapter_candidate`, `sycl_private_adapter_candidate`, or `non_goal`.
- [x] Require evidence for data transfer cost, memory residency, synchronization, stream/event usage, queue/profiler visibility, dependency burden, and scalar or RHI fallback.
- [x] Prefer backend-neutral RHI compute for runtime rendering/simulation workloads.
- [x] Permit CUDA/HIP/SYCL only for optional tooling or private adapters with clear install gates and scalar/RHI fallback paths.
- [x] Reject candidates where vendor runtime installation, backend availability, synchronization proof, queue/profiler visibility, or fallback evidence would make default validation fragile.

**Done When:** Every candidate has an explicit classification and no CUDA/HIP/SYCL runtime dependency has been introduced.

**Phase 7 candidate classification:**

| Candidate | Classification | Required proof before follow-up |
| --- | --- | --- |
| Runtime rendering/simulation compute | `rhi_compute` | Selected RHI backend/workload, queue family/type, timestamp/profiler evidence, explicit barriers/semaphores/fences/queue waits, and RHI fallback. |
| Offline cook/import/compression/analysis acceleration | `offline_tool_acceleration` | Deterministic input/output hashes, data transfer cost, memory residency, host install gate, legal/dependency records, and scalar fallback. |
| CUDA/HIP private adapter | `cuda_hip_private_adapter_candidate` | CUDA Toolkit or ROCm/HIP version, device properties including `asyncEngineCount` when relevant, stream/event synchronization, transfer/residency accounting, private adapter boundary, and scalar or RHI fallback. |
| SYCL private adapter | `sycl_private_adapter_candidate` | SYCL implementation/backend, device/aspect availability, queue profiling when timing is claimed, event synchronization, buffer/USM residency accounting, and scalar or RHI fallback. |
| Default runtime vendor compute | `non_goal` | Rejected for this milestone because default CUDA/HIP/SYCL runtime dependency, default validation dependency, public vendor handles, or duplicate backend ownership would make the engine host/vendor fragile. |

**Validation if implemented:**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
git diff --check
```

Expected: `optionalGpuComputeReview`, `currentOptionalGpuComputeReview`, and `host-optional-gpu-compute-review` are schema/static-check visible; CUDA/HIP/SYCL introduce no runtime dependency, no `vcpkg.json` feature, no CMake linkage, and no default validation dependency.

**Phase 7 validation evidence:**

| Date | Command | Result |
| --- | --- | --- |
| 2026-06-03 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` | Pass: composed `engine/agent/manifest.json`. |
| 2026-06-03 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Pass: `agent-config-check: ok`. |
| 2026-06-03 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Pass: `agent-manifest-compose: ok`, `json-contract-check: ok`. |
| 2026-06-03 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Pass: `ai-integration-check: ok`. |
| 2026-06-03 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Pass: `text-format-check: ok`, `format-check: ok`. |
| 2026-06-03 | `git diff --check` | Pass: no whitespace errors. |

Full `tools/validate.ps1` is not required for this Phase 7 closeout because the slice changes docs, schema, manifest fragments, composed manifest output, validation recipe descriptors, and static guards only; it does not change C++ runtime, build, packaging, or public API behavior.

## Phase 8: Long-Run Closeout And Next Selection

**Goal:** Close the milestone without broadening claims.

- [x] Update docs, plan registry, manifest fragments, composed manifest, schemas/static checks, and package validation expectations to match actual evidence.
- [x] Keep remaining non-claims explicit: Linux affinity, NUMA execution, broad SIMD, GPU async, CUDA/HIP/SYCL, cross-vendor parity, cross-backend parity, and broad optimization remain unclaimed unless an exact phase proves otherwise.
- [x] Run focused checks required by touched surfaces.
- [x] Do not run full validation for the closeout pointer sync because no C++/runtime/build/packaging/public-contract surfaces changed.
- [x] Run publication preflight before staging, push, PR, or merge.

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

**Phase 8 closeout evidence:**

| Date | Command | Result |
| --- | --- | --- |
| 2026-06-03 | PR #403 | Merged: Phase 0/1 long-run short-soak evidence contract. |
| 2026-06-03 | PR #406 | Merged: Phase 2 CPU profiling matrix and Phase 7 optional GPU compute review classifier, rebased onto `main`. |
| 2026-06-03 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` | Pass: composed `engine/agent/manifest.json`. |
| 2026-06-03 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Pass: `agent-config-check: ok`. |
| 2026-06-03 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Pass: `agent-manifest-compose: ok`, `json-contract-check: ok`. |
| 2026-06-03 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Pass: `ai-integration-check: ok`. |
| 2026-06-03 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Pass: `text-format-check: ok`, `format-check: ok`. |
| 2026-06-03 | `git diff --check` | Pass: no whitespace errors. |

Full `tools/validate.ps1` is not required for this Phase 8 closeout because the slice changes docs, manifest fragments, and composed manifest output only; it does not change C++ runtime, build, packaging, or public API behavior.

## Recommended Execution Order

1. Phase 0 only after operator selection.
2. Phase 1 long-run evidence contract.
3. Phase 2 Intel/AMD CPU Profiling Matrix v1.
4. After closeout, return to `next-production-gap-selection`.
5. Phase 6 GPU Async Evidence Gate only if renderer traces prove a selected backend/workload need.
6. Phase 3, Phase 4, or Phase 5 only when Phase 2 identifies a measured CPU-side need.

## Done When

- A selected package can run a deterministic long-run readiness smoke with bounded frame, memory, diagnostics, and shutdown evidence.
- Host-owned long soak recipes exist without making default validation slow or vendor-tool dependent.
- CPU profiling evidence decides whether Linux affinity, NUMA, broad SIMD, PGO/LTO, or data-layout follow-ups are justified.
- GPU async and vendor compute candidates are classified through backend-specific evidence instead of assumptions.
- Docs, manifest, static checks, and package evidence preserve exact claims and non-claims.
- `unsupportedProductionGaps` remains `[]` unless a concrete, documented production blocker is discovered.
