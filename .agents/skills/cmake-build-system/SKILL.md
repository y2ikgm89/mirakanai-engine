---
name: cmake-build-system
description: Adds CMake targets, presets, compiler options, tests, toolchains, and vcpkg wiring in this repository. Use when editing CMakeLists, CMakePresets.json, vcpkg.json, or bootstrap scripts.
paths:
  - "CMakeLists.txt"
  - "**/CMakeLists.txt"
  - "CMakePresets.json"
  - "vcpkg.json"
  - "cmake/**"
  - "docs/building.md"
  - "tools/bootstrap-deps.ps1"
  - "tools/check-toolchain.ps1"
---

# CMake Build System

## Rules

- Use target-based CMake.
- **`MK_tools`:** Keep implementation `.cpp` files under `engine/tools/{shader,gltf,asset,scene}/` as `OBJECT` targets (`MK_tools_shader`, `MK_tools_gltf`, `MK_tools_asset`, `MK_tools_scene`) aggregated by the `MK_tools` `STATIC` library in `engine/tools/CMakeLists.txt`. Each `OBJECT` target uses **cluster-minimal** `PUBLIC` `target_link_libraries`; **`MK_tools`** keeps the **full** `PUBLIC` `MK_*` set for consumer/install link closure. Do not reintroduce a flat `engine/tools/src/`. Cluster map and rules: `docs/specs/2026-05-11-directory-layout-target-v1.md`. When `.cpp` paths move, update `tools/check-json-contracts.ps1` and `tools/check-ai-integration.ps1` in the same task.
- Keep include paths scoped with `target_include_directories`.
- Give public libraries both `$<BUILD_INTERFACE:...>` and `$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>` include paths.
- Use `target_compile_features` for C++ standard requirements.
- Register tests with CTest.
- For registered desktop runtime games, prefer `PACKAGE_FILES_FROM_MANIFEST` so `game.agent.json.runtimePackageFiles` is the package file source of truth; do not mix it with literal `PACKAGE_FILES`.
- Keep local build output under `out/`.
- **Worktree cleanup:** Safe to delete for a clean rebuild: `out/`, Android Gradle/CXX outputs under `platform/android/` per `.gitignore`, `*.log`, `imgui.ini`, and (only when followed by `tools/bootstrap-deps.ps1`) `vcpkg_installed/`. **Never delete `external/vcpkg` or the entire `external/` directory as cache**—presets point `CMAKE_TOOLCHAIN_FILE` at `external/vcpkg/scripts/buildsystems/vcpkg.cmake`. Restore with `git clone https://github.com/microsoft/vcpkg.git external/vcpkg`, bootstrap `vcpkg.exe`, then `cmake --preset dev` or `tools/bootstrap-deps.ps1`.
- Keep project-wide build settings in checked-in `CMakePresets.json`; keep local developer overrides in ignored `CMakeUserPresets.json`.
- Use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1` to diagnose wrapper CMake, CTest, CPack, `clang-format`, Visual Studio, and MSBuild resolution before editing around build or format failures.
- Use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1 -RequireDirectCMake` when a task specifically depends on direct `cmake --preset ...` commands being available on `PATH`.
- Keep visible CMake build presets inheriting hidden `normalized-build-environment` so Windows direct `cmake --build --preset ...` runs collapse parent `PATH`/`Path` variants into one MSBuild child `Path`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1` validates this.
- Raw `clang-format --dry-run ...` commands also depend on `PATH`; prefer `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` / `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` unless `toolchain-check` reports `direct-clang-format-status=ready`.
- Use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1` for clang-tidy. It verifies `.clang-tidy`, configures the active CMake preset when needed, accepts native Makefile/Ninja `compile_commands.json`, or **synthesizes** `compile_commands.json` under the preset `binaryDir` from CMake File API codemodel data for the default Windows Visual Studio `dev` preset when needed; missing CMake or clang-tidy remains a local tool blocker.
- Keep Windows host diagnostics separate from build-toolchain readiness: Debugging Tools for Windows, Windows Graphics Tools, PIX on Windows, and Windows Performance Toolkit are official diagnostic tools, not CMake configure requirements.
- Keep installable SDK libraries in the root `MK_LIBRARY_TARGETS` list and export them through the `mirakana::` package namespace.
- Keep optional C++ package-manager dependencies in `vcpkg.json` manifest features.
- Preserve the official vcpkg `builtin-baseline` when editing dependencies; update it only as an explicit dependency-maintenance change with docs and notices.
- Use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1` before optional vcpkg-backed lanes. It owns manifest feature selection and package install/update.
- Keep vcpkg-enabled CMake presets configure-only: set `VCPKG_MANIFEST_INSTALL=OFF` and `VCPKG_INSTALLED_DIR=${sourceDir}/vcpkg_installed`.
- Do not put `VCPKG_MANIFEST_FEATURES` in CMake presets; select features in `tools/bootstrap-deps.ps1` so configure cannot restore packages implicitly.
- Keep `MK_CXX_STANDARD=23` as the required standard.
- Keep MSVC C++23 builds centralized through `MK_MSVC_CXX23_STANDARD_OPTION`. Use `/std:c++23preview` until a stable `/std:c++23` path is available through the supported CMake generator, then switch the cache default/presets to `/std:c++23`.
- Keep `MK_ENABLE_CXX_MODULE_SCANNING=ON` for development, release, sanitizer, Linux Clang CI, and optional vcpkg-backed presets; add project modules with CMake `FILE_SET CXX_MODULES`.
- Keep `MK_ENABLE_IMPORT_STD=ON` for those module-scanning presets, but only rely on `import std;` in targets when CMake reports `23` in `CMAKE_CXX_COMPILER_IMPORT_STD`.
- Keep reviewed CI exception presets explicit: `coverage` and `ci-macos-appleclang` set module scanning/import-std `OFF` because their current host compiler paths do not provide supported CMake module dependency scanning.
- Use the `cpp23-eval` presets and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/evaluate-cpp23.ps1` for C++23 verification checks.
- Use Context7 for current CMake, vcpkg, compiler, and SDK documentation before relying on memory for toolchain-specific behavior.

## Validation

During implementation, prefer a focused loop for the target being changed:

```powershell
cmake --build --preset dev --target <target>
ctest --preset dev --output-on-failure -R <test-name>
```

For completion, run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

For dependency changes, also run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1
```

For C++ standard policy changes, also run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-cpp-standard-policy.ps1
```

For static-analysis, public API boundary, or coverage changes, run the relevant command:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-coverage.ps1
```

If CMake or a compiler is missing, report the missing tool instead of editing around the build.

## Do Not

- Add global include directories.
- Add global compiler definitions unless every target needs them.
- Download dependencies during configure.
- Reintroduce configure-time vcpkg restore/install to hide a sandbox bootstrap failure.
- Remove `external/vcpkg` (or all of `external/`) while “cleaning” ignored paths; that breaks preset configure until the vcpkg clone is restored.

