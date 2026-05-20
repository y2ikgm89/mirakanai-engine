---
name: gameengine-cmake-build-system
description: Adds CMake targets, presets, compiler options, tests, toolchains, and vcpkg wiring in this repository. Use when editing CMakeLists, CMakePresets.json, vcpkg.json, or bootstrap scripts.
paths:
  - "CMakeLists.txt"
  - "**/CMakeLists.txt"
  - "CMakePresets.json"
  - "vcpkg.json"
  - "cmake/**"
  - "docs/building.md"
  - "tools/prepare-worktree.ps1"
  - "tools/bootstrap-deps.ps1"
  - "tools/check-toolchain.ps1"
---

# GameEngine CMake build system (Cursor)

Full workflow lives in shared skills. Read these canonical files (ASCII paths):

| Layer | Path |
| --- | --- |
| Claude Code | `.claude/skills/gameengine-cmake-build-system/SKILL.md` |
| Codex | `.agents/skills/cmake-build-system/SKILL.md` |
| Baseline | `AGENTS.md` |

Validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1`, `tools/cmake.ps1`, `tools/ctest.ps1`, and related wrappers in `AGENTS.md`.
Use CTest `RUN_SERIAL` only for time-sensitive host-resource tests; keep suite-wide automatic CMake/CTest parallelism enabled.
For C++23 evaluation, no switch or `-Debug` runs the Debug lane, `-Release` is release/package-only, and `-Debug -Release -Gui` is the full local confidence pass.
Engine/library targets keep preset-level CMake module scanning; non-module executable targets such as tests, probes, samples, and games opt out through `MK_disable_module_scanning_for_non_module_executable`.

MSVC targets use bounded `/MP2` plus `/Zf`, `COMPILE_PDB_OUTPUT_DIRECTORY` / `COMPILE_PDB_NAME` per target, and `/INCREMENTAL:NO` for linkable targets through `MK_apply_common_target_options`; `tools/cmake.ps1` clears stale `.tlog` directories with older aliased `.lastbuildstate` roots before Visual Studio builds to avoid MSB8028. See the canonical skills and `docs/building.md`.

**MK_tools CMake:** add sources under `engine/tools/{shader,gltf,asset,scene}/` (`OBJECT` targets with cluster-minimal links; `MK_tools` umbrella keeps full `PUBLIC` deps); see canonical skill and `docs/specs/2026-05-11-directory-layout-target-v1.md`.

**Coverage filters:** hosted `lcov` 2.x treats unmatched remove filters as errors. Keep optional `lcovRemovePatterns` guarded by `lcov --ignore-errors unused` and update `tools/check-coverage-thresholds.ps1` with coverage filter changes.

**Static analysis:** use `tools/check-tidy.ps1`, keep `.clang-tidy` `HeaderFilterRegex` absolute-path and Windows/Linux separator aware, keep strict CI on `--warnings-as-errors=*`, prefer `-Files` for focused local TUs and `-ShardCount` / `-ShardIndex` plus `-Jobs 0` for full hosted matrix lanes, and suppress only summary lines like `NN warnings generated.` after actionable diagnostics and exit codes are preserved. `tools/validate.ps1` runs independent static checks through bounded parallel jobs, prepares the File API query before its single `build.ps1` configure/build, runs that build with automatic CMake parallelism, reuses that reply for the tidy smoke unless `-SkipTidySmoke` is explicitly passed, then runs `test.ps1 -SkipBuild` with automatic CMake/CTest parallelism. Pass `-StaticJobs <N>` to `tools/validate.ps1` only when a local host needs a static-check throttle. Hosted CI uses `-StaticOnly -StaticJobs 1` for the dedicated static lane and `-SkipStaticChecks -SkipTidySmoke` for the Windows MSVC lane; Windows build caches restore the newest compatible tree with `actions/cache/restore` and save a fresh SHA-keyed cache with `actions/cache/save` only after success. Keep local slice gates on the default full path unless a narrower check is justified. Pass `-Jobs <N>` to `tools/build.ps1` or `tools/test.ps1` only when a local host needs an explicit lower throttle.

**Worktree setup/cleanup:** After manual `git worktree add`, run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/prepare-worktree.ps1` inside the linked worktree. It verifies ignored worktree roots and links an existing local `external/vcpkg` checkout plus any existing local `vcpkg_installed/` package tree. Guarded post-merge cleanup accepts standard roots from Git main worktree porcelain records, unlinks those worktree-local reparse points before Git removes the task worktree, and must not follow or delete shared targets. Deleting `out/` is fine. **Do not delete `external/vcpkg`** (Microsoft vcpkg clone; `CMAKE_TOOLCHAIN_FILE` in presets). If it is missing, `git clone https://github.com/microsoft/vcpkg.git external/vcpkg`, bootstrap `vcpkg.exe`, then `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev` or `tools/bootstrap-deps.ps1`. Optional: remove `vcpkg_installed/` only when you will rerun `tools/bootstrap-deps.ps1`. Visible configure/build presets inherit `normalized-configure-environment` / `normalized-build-environment`; presets normalize raw `cmake --preset ...` PATH/Path behavior, and `tools/cmake.ps1` / `tools/ctest.ps1` additionally normalize the launched child process for local loops. Do not repair generated `out/build/<preset>` trees by editing `CMakeCache.txt`, `.vcxproj`, `.sln`, or MSBuild props; rerun the supported configure wrapper/preset, or verify and remove only that disposable preset tree under `out/build/`.
