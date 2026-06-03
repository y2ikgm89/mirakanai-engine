# AVX2 Reviewed Target Execution v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` or focused inline execution. Steps use checkbox (`- [ ]`) syntax for tracking.

**Status:** Active.

**Goal:** Add a reviewed AVX2 CPU SIMD execution lane for the existing `MK_core` dot-product evidence without making AVX2 a global engine build or runtime requirement.

**Architecture:** `mirakana_core` remains a portable baseline target. A new private `mirakana_core_avx2` `OBJECT` target compiles only `engine/core/src/simd_dispatch_avx2.cpp` with target-local AVX2 compiler options, then contributes its object file to `mirakana_core` through `$<TARGET_OBJECTS:...>`. Runtime dispatch selects AVX2 only when the reviewed object target is compiled, CPUID/XGETBV runtime support is present, and the request permits the lane; scalar and SSE2 remain fail-closed fallbacks.

**Tech Stack:** C++23, `MK_core`, CMake target-scoped compile options, CMake `OBJECT` libraries, MSVC `/arch:AVX2`, GCC/Clang `-mavx2`, x86/x64 CPUID/XGETBV feature detection, AVX2 intrinsics in a private translation unit, `sample_desktop_runtime_game`, PowerShell validation wrappers.

---

## Official References

- CMake `add_library(OBJECT)` and `$<TARGET_OBJECTS:...>` docs: object libraries compile source files without archiving, and their object files can be consumed by another target.
- CMake `target_compile_options` docs: compiler options are target-scoped, which keeps AVX2 flags out of unrelated engine targets.
- Microsoft Learn `/arch` x64: `/arch:AVX2` enables Intel Advanced Vector Extensions 2 and the docs instruct checking CPU support before executing extension-specific code.
- Microsoft Learn `__cpuid` / `__cpuidex`: official MSVC intrinsics for x86/x64 CPU feature leaves, including AVX/AVX2 feature examples.
- GCC x86 options: `-mavx2` enables AVX2 built-ins and code generation for x86 targets.
- Clang command-line reference: `-mavx2` / `-mno-avx2` are target-dependent X86 options.
- Intel Intrinsics Guide: official AVX/AVX2 intrinsic reference for `_mm256_*` operations and performance metadata.

## Context

- `unsupportedProductionGaps = []`; this is a post-1.0 optimization candidate, not a reopened 1.0 blocker.
- SIMD Dispatch Policy And Evidence v1 already exposes scalar/SSE2 value-only evidence and explicitly leaves AVX2 reviewed target execution unclaimed.
- Existing `engine/core/src/simd_dispatch.cpp` intentionally fails closed for AVX2 through `reviewed_target_gate_missing`.
- The previous package smoke required `simd_dispatch_policy_avx2_selected=0`; this plan updates that validator to allow `avx2` only when the reviewed target and runtime support are both proven.

## Constraints

- Do not add global `/arch:AVX2`, `-mavx2`, `/arch:AVX512`, or `-march=native` to `mirakana_core` or the whole engine.
- Do not execute AVX2 instructions on hosts that lack AVX2 CPU support or OS YMM register support.
- Do not expose native CPU handles, raw owning pointers, compiler-specific types, or backend state in public APIs.
- Do not claim broad Intel/AMD tuning, broad SIMD quality, ARM NEON, NUMA placement, Linux affinity, GPU async overlap, CUDA, HIP, SYCL, allocator enforcement, automatic/LRU GPU residency, cross-vendor parity, or cross-backend parity.
- Keep non-x86 and non-AVX2 hosts green through scalar/SSE2 fallback and deterministic tests.

## Files

- Modify: `engine/core/CMakeLists.txt`
- Modify: `engine/core/src/simd_dispatch.cpp`
- Create: `engine/core/src/simd_dispatch_avx2.cpp`
- Create: `engine/core/src/simd_dispatch_detail.hpp`
- Modify: `tests/unit/core_tests.cpp`
- Modify: `games/sample_desktop_runtime_game/main.cpp`
- Modify: `tools/validate-desktop-game-runtime.ps1`
- Modify: `tools/validate-installed-desktop-runtime.ps1`
- Modify: `tools/check-json-contracts-010-engine-manifest.ps1`
- Modify: `tools/check-json-contracts-030-tooling-contracts.ps1`
- Modify: `tools/check-json-contracts-040-agent-surfaces.ps1`
- Modify: `tools/check-json-contracts-050-generated-games.ps1`
- Modify: `tools/check-ai-integration-010-agent-baseline.ps1`
- Modify: `tools/check-ai-integration-020-engine-manifest.ps1`
- Modify: `tools/check-ai-integration-030-runtime-rendering.ps1`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `engine/agent/manifest.fragments/014-gameCodeGuidance.json`
- Compose output: `engine/agent/manifest.json`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`

## Task 1: Select Active Plan And Static Contract

- [x] Set `currentActivePlan` to this file and `recommendedNextPlan.id` to `avx2-reviewed-target-execution-v1`.
- [x] Add static-check branches for `avx2-reviewed-target-execution-v1` so `check-ai-integration` and `check-json-contracts` validate the selected candidate text instead of falling through to legacy defaults.
- [x] Compose `engine/agent/manifest.json`.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
```

Expected: both pass after the selected active-plan text and static needles agree.

## Task 2: Add AVX2 Target-Local Build Lane

- [x] Update `engine/core/CMakeLists.txt` with a private `mirakana_core_avx2` `OBJECT` target only when the host processor is x86/x64 and the compiler is MSVC, GNU, or Clang.
- [x] Apply `/arch:AVX2` only to `mirakana_core_avx2` for MSVC-like builds.
- [x] Apply `-mavx2` only to `mirakana_core_avx2` for GNU/Clang builds.
- [x] Add `$<TARGET_OBJECTS:mirakana_core_avx2>` to `mirakana_core` sources.
- [x] Keep `mirakana_core` itself on the normal project compile options.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
```

Expected: configure succeeds and generated build files contain the new object target without changing global compile options.

## Task 3: Implement Private AVX2 Kernel

- [x] Create `engine/core/src/simd_dispatch_avx2.cpp` and private declaration header `engine/core/src/simd_dispatch_detail.hpp`.
- [x] Define the private AVX2 cross-TU kernel in `namespace mirakana::detail`:

```cpp
[[nodiscard]] float compute_avx2_dot_product(std::span<const float> lhs, std::span<const float> rhs) noexcept;
```

- [x] Keep reviewed target availability and XGETBV runtime probing in the baseline `simd_dispatch.cpp` TU so no AVX2-compiled function is needed before runtime gates pass.
- [x] In the AVX2-enabled branch, include `<immintrin.h>` and compute eight `float` lanes at a time with `_mm256_loadu_ps`, `_mm256_mul_ps`, `_mm256_add_ps`, and `_mm256_storeu_ps`.
- [x] In the non-AVX2 branch, compute with a local scalar loop so accidental calls remain deterministic.
- [x] Do not add public headers for the private kernel.

## Task 4: Promote Dispatch Policy To Reviewed AVX2

- [x] Update `observe_cpu_simd_features()` so `features.avx2_compile_supported` means the reviewed AVX2 object target is available, not that the baseline TU was globally compiled with AVX2.
- [x] Update auto-selection to prefer AVX2 over SSE2 only when compile and runtime support are both true.
- [x] Remove the permanent `reviewed_target_gate_missing` block for AVX2 when the reviewed target is available.
- [x] Set `policy.avx2_selected=true` only for selected AVX2 dispatch.
- [x] Dispatch AVX2 selected evidence to `detail::compute_avx2_dot_product`.
- [x] Preserve scalar/SSE2 fallback and diagnostics for missing compile/runtime support.

## Task 5: Tests

- [x] Replace the old fail-closed AVX2 test with a policy-selection test that proves requested AVX2 selects only when the reviewed compile and observed runtime gates are true, and matches scalar when actual runtime support allows execution.
- [x] Add a deterministic injected-feature test proving AVX2 request fails closed with `compile_lane_unavailable` when compile support is unavailable.
- [x] Update the auto-select test to prove AVX2 outranks SSE2 when both are available, while remaining safe on hosts without actual AVX2 runtime support.
- [x] Keep the runtime-observed SSE2 test tolerant of hosts without SSE2 support.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_core_tests
```

Expected: `MK_core_tests` passes on the local host.

## Task 6: Package Evidence

- [x] Update `simd_dispatch_policy_ready()` so selected AVX2 is allowed when the policy is ready, input/result counters match, and side-effect flags remain zero.
- [x] Emit a new package-visible boolean field `simd_dispatch_policy_reviewed_avx2_target_available`.
- [x] Update `tools/validate-installed-desktop-runtime.ps1` so `selected_lane` accepts `scalar|sse2|avx2` and enforces coherent fallback flags for each lane.
- [x] Update `tools/validate-desktop-game-runtime.ps1` so the desktop-runtime multi-config CTest invocation passes `-C Debug`.
- [x] Update generated-game/static package needles for the new AVX2-selected boundary.
- [x] Run source-tree desktop runtime smoke:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset desktop-runtime
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset desktop-runtime --target sample_desktop_runtime_game
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset desktop-runtime -C Debug --output-on-failure -R sample_desktop_runtime_game_smoke
```

Expected: smoke passes and reports scalar, SSE2, or AVX2 coherently for the host.

## Task 7: Docs, Manifest, And Drift Check

- [x] Update `docs/current-capabilities.md`, `docs/ai-game-development.md`, and `docs/roadmap.md` from "AVX2 reviewed target execution unclaimed" to the new narrow ready boundary after evidence lands.
- [x] Update `engine/agent/manifest.fragments/004-modules.json` and `014-gameCodeGuidance.json` to describe the reviewed AVX2 target without broad CPU/GPU optimization claims.
- [x] Compose `engine/agent/manifest.json`.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
```

Expected: all pass after docs, manifest, and static checks agree.

## Task 8: Slice Validation And Publication

- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files "engine/core/src/simd_dispatch.cpp,engine/core/src/simd_dispatch_avx2.cpp,tests/unit/core_tests.cpp"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Preset desktop-runtime -Files "games/sample_desktop_runtime_game/main.cpp"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Expected: focused checks, package smoke, and full validation pass, with known host-gated Apple/Metal diagnostics only when applicable.

- [x] Run publication preflight:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1 -Branch codex/avx2-reviewed-target-dispatch-v1
```

- [x] Commit task-owned files with validation evidence.
- [x] Push branch and create PR #397.
- [ ] Wait for hosted checks including `PR Gate` and `Windows MSVC`, merge with `--match-head-commit`, verify the merged head reaches `origin/main`, and run guarded worktree cleanup.

## Validation Evidence

- Passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev`
- Passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests`
- Passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_core_tests`
- Passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset desktop-runtime`
- Passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset desktop-runtime --target sample_desktop_runtime_game`
- Passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset desktop-runtime -C Debug --output-on-failure -R sample_desktop_runtime_game_smoke`
- Passed: `out\build\desktop-runtime\games\Debug\sample_desktop_runtime_game\sample_desktop_runtime_game.exe --smoke --require-simd-dispatch-policy`; local status reported `simd_dispatch_policy_selected_lane=avx2`, reviewed target available, AVX2 runtime supported, `simd_dispatch_policy_dot_product_result=120`, no raw pointer retention, and zero native handle/NUMA/GPU async/CUDA/HIP/SYCL side effects.
- Passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`
- Passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`
- Passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`
- Passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`
- Passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`
- Passed: focused `tools/check-tidy.ps1` for `engine/core/src/simd_dispatch.cpp`, `engine/core/src/simd_dispatch_avx2.cpp`, and `tests/unit/core_tests.cpp` using a comma-joined absolute file list.
- Passed: focused `tools/check-tidy.ps1 -Preset desktop-runtime` for `games/sample_desktop_runtime_game/main.cpp`.
- Passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`; 18/18 selected desktop-runtime tests passed.
- Passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game`; installed package validation reported `simd_dispatch_policy_selected_lane=avx2`, reviewed target available, AVX2 runtime supported, `simd_dispatch_policy_avx2_selected=1`, and package generation succeeded.
- Passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`; 19 static checks passed, diagnostic-only Apple/Metal host gates were reported on Windows, build passed, tidy smoke passed, and 85/85 dev CTest tests passed.
- Corrected: hosted PR #397 Linux Clang ASan/UBSan first run failed only `MK_core_tests` on `result.worker_wait_count > 0`; SIMD tests passed. The work-stealing test now treats idle waits as scheduler-timing dependent while preserving positive steal success and coherent published counters.
- Passed after hosted ASan hardening: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests`
- Passed after hosted ASan hardening: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_core_tests`
- Passed after hosted ASan hardening: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`
- Passed after hosted ASan hardening: focused `tools/check-tidy.ps1` for `tests/unit/core_tests.cpp`.
- Passed after hosted ASan hardening: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`; 19 static checks passed, diagnostic-only Apple/Metal host gates were reported on Windows, build passed, tidy smoke passed, and 85/85 dev CTest tests passed.

## Done When

- `MK_core` provides scalar, SSE2, and reviewed AVX2 dot-product evidence without making AVX2 global.
- AVX2 dispatch is selected only when reviewed target compile support and runtime CPU/OS support are both true.
- Package smoke reports coherent `simd_dispatch_policy_*` counters for scalar/SSE2/AVX2 and keeps native handles, NUMA, GPU async overlap, CUDA, HIP, and SYCL side effects at zero.
- Docs, manifest, static checks, focused tests, package smoke, full validation, hosted checks, PR merge, and worktree cleanup are complete.
