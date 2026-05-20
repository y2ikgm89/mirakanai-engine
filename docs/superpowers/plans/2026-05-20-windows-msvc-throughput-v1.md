# Windows MSVC Throughput v1 (2026-05-20)

**Plan ID:** `windows-msvc-throughput-v1`

## Goal

Improve Windows MSVC build and hosted validation wall-clock time without reducing the default local validation gate, static-analysis coverage, or GitHub Flow safety.

## Context

- CMake build/test wrappers already use official `cmake --build --parallel` and CTest parallelism.
- MSBuild project-level parallelism is already active through CMake's Visual Studio generator build path.
- Prior MSVC PDB and `.tlog` contention fixes are complete; this slice should preserve per-target compiler PDB isolation, wrapper build serialization, `/INCREMENTAL:NO`, and normal MSBuild parallelism.
- Microsoft documents `/MP` for compiler-level parallel compilation, with `P x C` oversubscription risk when combined with MSBuild `/m`, and `/Zf` for faster PDB generation when multiple `cl.exe` processes run. This plan uses a bounded `/MP2` default instead of an unbounded `/MP`.
- The hosted Windows MSVC lane currently duplicates the clang-tidy smoke even though `Agent Static Guards` and `Full Repository Static Analysis` own static evidence.

## Constraints

- Do not remove local default `tools/validate.ps1` coverage.
- Do not add global `/FS`; keep PDB isolation and wrapper build serialization as the first-line C1041 solution.
- Do not disable Windows MSVC build or CTest coverage.
- Keep `unsupportedProductionGaps = []`; this is a docs/governance/toolchain slice, not a new production blocker.

## Phases

### Phase 1 - MSVC compile throughput and CI duplicate-smoke removal

- [x] Add RED static guards for `/MP2`, `/Zf`, and Windows CI `-SkipTidySmoke`.
- [x] Add bounded `MK_MSVC_MULTIPROCESSOR_COMPILE_PROCESSES=2`, emit `/MP2`, and explicitly pass `/Zf` in `MK_apply_common_target_options`.
- [x] Add `tools/validate.ps1 -SkipTidySmoke` and use it only in the hosted Windows MSVC lane with `-SkipStaticChecks`.
- [x] Synchronize docs, skills, subagents, rules, manifest fragments, and static guards.
- [x] Run focused CMake/tooling checks and full `tools/validate.ps1`.
- [x] Commit, push, and open/update a PR.

## Done When

- `CMakeLists.txt` emits `/MP2` and `/Zf` for MSVC targets.
- Default `tools/validate.ps1` still runs the tidy smoke; only explicit `-SkipTidySmoke` skips it.
- Hosted Windows MSVC uses `-SkipStaticChecks -SkipTidySmoke`; static lanes still own full static evidence.
- `tools/check-ci-matrix.ps1`, `tools/check-ai-integration.ps1`, `tools/check-json-contracts.ps1`, `tools/check-agents.ps1`, and full `tools/validate.ps1` pass.
- The checkpoint is committed and published through a topic-branch PR.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | RED | Failed after adding guards because `AGENTS.md` lacked `/MP2`. |
| Context7 `/kitware/cmake` and Microsoft Learn docs | PASS | Confirmed official `cmake --build --parallel` / `CMAKE_BUILD_PARALLEL_LEVEL`, MSBuild `/m`, MSVC `/MP`, and `/Zf` guidance. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/prepare-worktree.ps1` | PASS | Linked worktree has ready `external/vcpkg` and `vcpkg_installed` links. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1` | PASS | CMake 3.31.6, MSBuild 17.14, Visual Studio Build Tools, linked-worktree readiness. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev` | PASS | Dev preset regenerated Visual Studio project files. |
| `Select-String out\build\dev\MK_runtime_tests.vcxproj -Pattern 'MultiProcessorCompilation','ProcessorNumber','/Zf'` | PASS | Generated project maps `/MP2` to `MultiProcessorCompilation=true` and `ProcessorNumber=2`; `/Zf` remains in `AdditionalOptions`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_tests --parallel 2` | PASS | Focused MSVC build passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_tests` | PASS | Focused runtime tests passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | RED | Initial full gate exposed clang-tidy File API synthesis passing MSVC-only `/Zf` to clang-tidy. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -MaxFiles 1 -ReuseExistingFileApiReply` | PASS | After filtering build-only MSVC `/MP*` and `/Zf` from synthesized tidy compile commands. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | Agent-surface drift guard passed after adding `/MP2`, `/Zf`, `-SkipTidySmoke`, and tidy sanitizer needles. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1` | PASS | Windows MSVC hosted lane uses `-SkipStaticChecks -SkipTidySmoke`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | Manifest compose and JSON contracts passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS | Agent instruction and skill budget checks passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full gate passed: static checks, MSVC build, generated MSVC C++23 check, tidy smoke, and 66/66 CTest tests. |
