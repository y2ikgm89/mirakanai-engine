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

# CMake Build System

## Scope

Use this skill for CMake targets, presets, compiler options, tests, install/export wiring, vcpkg manifest/bootstrap policy, and build or static-analysis wrapper changes.

## Build Rules

- Use target-based CMake: scoped `target_include_directories`, `target_link_libraries`, `target_compile_features`, and target-local compile options/definitions.
- Keep public libraries installable with `$<BUILD_INTERFACE:...>` and `$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>`; export SDK libraries through the `mirakana::` package namespace.
- Use C++23. Keep `MK_CXX_STANDARD=23`, `MK_MSVC_CXX23_STANDARD_OPTION`, and project modules through `FILE_SET CXX_MODULES`; gate `import std;` on supported CMake/compiler reports.
- For registered desktop runtime games, prefer `PACKAGE_FILES_FROM_MANIFEST` so `game.agent.json.runtimePackageFiles` is the package-file source of truth.
- Keep `MK_tools` implementation `.cpp` files under `engine/tools/{shader,gltf,asset,scene}/` as cluster-minimal `OBJECT` targets aggregated by `MK_tools`; do not reintroduce a flat `engine/tools/src/`.
- For performance flags, ISA-specific code, PGO, LTO, or release-performance presets, also use the performance optimization skill and keep flags target-local or preset-specific.

## Toolchain Entrypoints

- Diagnose wrapper CMake, CTest, CPack, clang-format, Visual Studio, and MSBuild with `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1`.
- Use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1 -RequireDirectCMake` only when raw `cmake --preset ...` availability is the issue; otherwise prefer `tools/cmake.ps1` and `tools/ctest.ps1`.
- Use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1 -RequireVcpkgToolchain` for vcpkg-backed configure gates.
- Use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/prepare-worktree.ps1` after manual linked worktree creation so ignored local `external/vcpkg` and `vcpkg_installed/` roots are linked safely.
- Use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1` for optional vcpkg packages; CMake configure must not restore or install packages.
- Prefer `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` / `tools/format.ps1`; raw clang-format depends on `direct-clang-format-status=ready`.

## Preset And Cache Policy

- Keep checked-in settings in `CMakePresets.json`; keep local overrides in ignored `CMakeUserPresets.json`.
- Preserve hidden `normalized-configure-environment` and `normalized-build-environment`; presets inherit `PATH` and unset `Path`, while wrappers collapse parent variants into child `Path` on Windows.
- Keep local build output under `out/`.
- Do not repair generated `out/build/<preset>` trees by editing `CMakeCache.txt`, `.vcxproj`, `.sln`, or MSBuild props. Recover by rerunning `tools/cmake.ps1 --preset <preset>`; for clean rebuilds, verify the resolved target is under `out/build/` before deleting only that disposable tree.
- Safe cleanup targets: `out/`, Android Gradle/CXX outputs under `platform/android/`, `*.log`, `imgui.ini`, and `vcpkg_installed/` only when followed by bootstrap. Never delete `external/vcpkg` or the whole `external/` directory as cache.

## Static And CI Policy

- `tools/validate.ps1` runs independent static checks through bounded parallel jobs, prepares CMake File API data, builds with automatic CMake/CTest parallelism, and then runs tests through `test.ps1 -SkipBuild`.
- Pass `-StaticJobs <N>`, `-StaticCheckTimeoutSeconds <N>`, or build/test `-Jobs <N>` only for host-specific throttling; hosted lanes use explicit policy, including `-StaticOnly -StaticJobs 1 -StaticCheckTimeoutSeconds 120`, `-SkipStaticChecks`, and `-SkipTidySmoke`.
- CI caches use `actions/cache/restore` and `actions/cache/save`; restore transport failures fall back to clone/bootstrap/build, and save is non-blocking after success.
- clang-tidy policy: keep `HeaderFilterRegex` path-aware, keep hosted `--warnings-as-errors=*`, use `-ShardCount` / `-ShardIndex` plus `-Jobs 0` in full hosted matrix lanes, use `-Files` for focused local loops, and suppress only summary text such as `NN warnings generated.`.
- Coverage policy: keep hosted `lcov --ignore-errors unused` compatibility for optional remove filters and update `tools/check-coverage-thresholds.ps1` when threshold/filter contracts change.

## vcpkg And Dependencies

- Keep optional C++ dependencies in `vcpkg.json` manifest features and preserve the official `builtin-baseline` unless doing explicit dependency maintenance with docs/notices.
- Keep vcpkg-enabled presets configure-only with `VCPKG_MANIFEST_INSTALL=OFF` and `VCPKG_INSTALLED_DIR=${sourceDir}/vcpkg_installed`.
- Do not put `VCPKG_MANIFEST_FEATURES` in CMake presets; feature selection belongs to `tools/bootstrap-deps.ps1`.

## MSVC And Modules

- Keep MSVC throughput on bounded `/MP2` plus `/Zf`, compact CMake target names for long single-source tests, `COMPILE_PDB_OUTPUT_DIRECTORY`, `COMPILE_PDB_NAME`, and `/INCREMENTAL:NO` through common target options.
- Keep Visual Studio builds on `tools/cmake.ps1`; the wrapper serializes overlapping builds and clears stale `.tlog` roots to avoid MSB8028.
- Keep module scanning on for development/release/sanitizer/Linux Clang/vcpkg-backed presets, off for non-module executables through the central helper, and off for reviewed exception presets such as tidy/coverage/Apple host lanes.

## Host Diagnostics

- Windows diagnostics are host tools, not configure requirements: Debugging Tools for Windows, Windows Graphics Tools, PIX on Windows, and Windows Performance Toolkit.
- Use Context7 for current CMake, vcpkg, compiler, and SDK documentation before relying on memory for toolchain-specific behavior.

## Validation

Focused loop:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target <target>
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R <test-name>
```

Completion or relevant policy changes:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-cpp-standard-policy.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-coverage.ps1
```

If CMake, clang-format, clang-tidy, vcpkg, or a compiler is missing, report the missing tool instead of editing around the build.
