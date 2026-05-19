# Build Toolchain MSVC PDB Isolation v1 (2026-05-20)

**Plan ID:** `build-toolchain-msvc-pdb-isolation-v1`
**Status:** Completed.
**Current pointer rule:** This is a short build/toolchain reliability slice and does not replace the engine capability `currentActivePlan`.

## Goal

Remove recurring MSVC compile-PDB `C1041` contention from supported local build workflows without reducing normal single-build MSBuild parallelism.

## Context

- Several prior Windows dev builds hit transient `C1041` when overlapping CMake/MSBuild invocations wrote to the same compiler PDB.
- Microsoft documents `/FS` as a serialization option, but also warns it can significantly lengthen builds and recommends separate intermediate/output locations or serialized project builds instead.
- CMake exposes compiler-generated PDB target properties through `COMPILE_PDB_OUTPUT_DIRECTORY` and `COMPILE_PDB_NAME`, which map to the compiler `/Fd` PDB path rather than linker PDB output.

## Constraints

- Do not add global `/FS` unless a measured follow-up proves it is required.
- Preserve normal MSBuild target-level parallelism inside one build invocation.
- Do not edit generated `.vcxproj`, `.sln`, or `CMakeCache.txt` files by hand.
- Keep Codex/Claude/Cursor build guidance and build-fixer subagents aligned.

## Done When

- `MK_apply_common_target_options` gives MSVC targets per-target compiler PDB names/directories.
- Supported `tools/*.ps1` CMake build entrypoints serialize overlapping build invocations against one clone, while one build still runs with normal generator parallelism.
- Agent-facing build guidance, manifest fragments, and static checks describe the policy.
- Focused generated-project evidence shows distinct `ProgramDataBaseFileName` paths for representative targets.
- Static checks and one full validation gate pass.

## Validation Evidence

- RED static guard: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed before implementation because `AGENTS.md` did not contain `COMPILE_PDB_OUTPUT_DIRECTORY`.
- Official docs checked: [CMake `COMPILE_PDB_OUTPUT_DIRECTORY`](https://cmake.org/cmake/help/latest/prop_tgt/COMPILE_PDB_OUTPUT_DIRECTORY.html), [CMake `COMPILE_PDB_NAME`](https://cmake.org/cmake/help/latest/prop_tgt/COMPILE_PDB_NAME.html), Microsoft [`/Fd`](https://learn.microsoft.com/en-us/cpp/build/reference/fd-program-database-file-name?view=msvc-170), and Microsoft [`/FS`](https://learn.microsoft.com/ja-jp/cpp/build/reference/fs-force-synchronous-pdb-writes?view=msvc-170). Microsoft documents `/FS` as serialization that can significantly lengthen builds and recommends separate intermediate/output locations or serialized project builds; this slice uses CMake target PDB separation first and only serializes overlapping wrapper build invocations.
- Focused configure: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev` passed.
- Generated project evidence: `mirakana_ui.vcxproj`, `MK_core_tests.vcxproj`, and `sample_ui_audio_assets.vcxproj` now emit distinct compiler `ProgramDataBaseFileName` paths under `out/build/dev/pdb/<target>/<config>/<target>.pdb`.
- Overlap smoke: concurrent `tools/cmake.ps1 --build --preset dev --target MK_core_tests` and `--target sample_ui_audio_assets` queued through the `cmake-build` mutex and both passed without `C1041`.
- Static checks: `tools/check-agents.ps1`, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, and `tools/check-format.ps1` passed.
- Full phase gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with `validate: ok`; `production-readiness-audit` reported `unsupported_gaps=0`.
