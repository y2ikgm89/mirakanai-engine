# Validate Parallel Build/Test v1 (2026-05-20)

**Plan ID:** `validate-parallel-build-test-v1`
**Status:** Completed docs/governance slice.

## Goal

Reduce local `tools/validate.ps1` wall-clock time without reducing validation coverage, diagnostics, or the requirement to run full validation at C++/runtime/build/public-contract slice gates.

## Context

- `validate-single-configure-v1` removed the duplicate configure/build pass by reusing the CMake File API reply and running `test.ps1 -SkipBuild`.
- The remaining default build/test path can still leave official CMake/CTest parallelism unused when repository wrappers call CMake/CTest without `--parallel`.
- Context7 official CMake documentation confirms `cmake --build --parallel <jobs>` and CTest/test-preset `execution.jobs`/`ctest --parallel` are the supported parallel execution surfaces. The repository wrapper path is the cleanest place to apply dynamic host CPU-count defaults because checked-in presets cannot express a portable processor-count value.

## Constraints

- Keep every existing `validate.ps1` check; do not add a fast mode or skip mode.
- Keep overlapping CMake build invocation serialization and MSVC per-target PDB isolation intact.
- Keep host-resource or file-system watcher tests honest under CTest parallelism by marking only the affected CTest entries serial.
- Preserve focused loops and one full `validate.ps1` at coherent build/toolchain/public-contract gates.
- Update agent surfaces and manifest fragments because validation workflow behavior changes.

## Phase 1: Automatic Wrapper Parallelism

**Status:** Completed.

### Done When

- RED static guard fails first for missing wrapper/default parallelism contract.
- `tools/build.ps1` configures once, then runs `cmake --build --preset dev --parallel <jobs>` with `-Jobs 0` resolving to host processor count.
- `tools/test.ps1` builds with the same CMake parallelism unless `-SkipBuild` is used, then runs CTest with `--parallel <jobs>`.
- Docs, skills, subagents, manifest fragments, composed manifest, and static checks describe the clean wrapper contract.
- Hosted Windows MSVC file-watcher contention is resolved through CTest `RUN_SERIAL` for `MK_platform_process_tests`, not by disabling suite-wide parallelism.
- Focused checks and full `tools/validate.ps1` pass.

## Validation Evidence

| Check | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` after RED guard | FAIL | Proved the static guard caught missing `tools/build.ps1` / `tools/test.ps1` automatic parallelism and manifest text before implementation. |
| Context7 CMake documentation lookup | PASS | Confirmed official CMake/CTest `--parallel` and test preset `execution.jobs` semantics; wrapper-owned dynamic CPU-count defaults avoid non-portable preset expressions. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Default wrapper resolved `-Jobs 0` to 32 and ran `cmake --build --preset dev --parallel 32`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1 -SkipBuild` | PASS | Default wrapper resolved `-Jobs 0` to 32 and ran 65/65 CTest tests with `--parallel 32` in 25.21s. |
| `Invoke-ScriptAnalyzer` on `tools/build.ps1`, `tools/test.ps1` | PASS | No warnings or errors on the changed wrappers after replacing new `Write-Host` calls with `Write-Information`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1 -Jobs 2` | PASS | Explicit throttle path passed and reported `cmake parallel jobs=2`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1 -SkipBuild -Jobs 2` | PASS | Explicit throttle path passed 65/65 CTest tests and reported `ctest parallel jobs=2`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS | Agent surface size budgets and parity checks passed after shortening `AGENTS.md`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | Composed manifest stayed in sync with fragments. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | Guarded wrapper/docs/manifest/skill contract passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Text and C++ formatting checks passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full validation passed; `production-readiness-audit: unsupported_gaps=0`; expected Windows host-gated diagnostics for Apple/Metal remained diagnostic-only. The validate run also proved `test.ps1 -SkipBuild` is now invoked directly instead of through array splatting that cannot carry named switches once `-Jobs` exists. |
| Hosted PR #131 Windows MSVC check at head `36af827d` | FAIL | CTest parallelism exposed `MK_platform_process_tests` / Windows native file watcher as time-sensitive under concurrent hosted load: `events=0`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` after adding the RUN_SERIAL static guard | FAIL | Proved the guard caught missing `set_tests_properties(MK_platform_process_tests PROPERTIES RUN_SERIAL TRUE)` before the CMake fix. |
| Context7 CTest documentation lookup | PASS | Confirmed official CTest `RUN_SERIAL` is the intended property for tests that must not execute concurrently with other tests. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` after `RUN_SERIAL` fix | PASS | Regenerated CTest files and kept default `cmake --build --parallel 32`. |
| `Select-String out/build/dev/CTestTestfile.cmake -Pattern 'MK_platform_process_tests|RUN_SERIAL'` | PASS | Generated CTest metadata contains `RUN_SERIAL "TRUE"` for `MK_platform_process_tests`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1 -SkipBuild` after `RUN_SERIAL` fix | PASS | Parallel CTest still ran 65/65 tests successfully; `MK_platform_process_tests` ran after the parallel group and passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` after `RUN_SERIAL` fix | PASS | Full validation passed in 77.4s with automatic CMake/CTest parallelism; `production-readiness-audit: unsupported_gaps=0`; Apple/Metal diagnostics remained expected host-gated checks. |
