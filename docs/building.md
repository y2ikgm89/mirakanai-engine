# Building, Installing, And CMake Packages

This document records the repository default **CMake preset** flow, `cmake --install`, and the assumptions for consumers using **`find_package(Mirakanai)`**. Keep the C++ modules and `import std` matrix aligned with [C++ standard policy](cpp-standard.md) and the root `CMakeLists.txt` configure log.

## Prerequisites

- **CMake 3.30 or newer**, aligned with `CMakePresets.json` `cmakeMinimumRequired`.
- **C++23 only**. `MK_CXX_STANDARD` accepts `23` only.
- Do not add dummy install targets or standard-version compatibility shims. This follows the constraints from `cmake-install-export-and-cxx-modules-audit-v1`.

## Configure And Build

Prefer the preset-driven flow:

```powershell
cmake --preset dev
cmake --build --preset dev
ctest --preset dev --output-on-failure
```

Use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` for the aggregated agent/CI validation gate. Use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1` to diagnose missing local tools.

On Windows, checked-in CMake build presets inherit `normalized-build-environment` and repository wrappers pass MSBuild a single `PATH` environment key even when the parent process exposes both `PATH` and `Path`. Keep new visible build presets inheriting that hidden preset.

Checked-in CMake configure presets also inherit `normalized-configure-environment` so direct `cmake --preset ...` uses the same single child `PATH` policy and removes duplicate `Path` before CMake try-compile or generator tool checks run. Keep new visible configure presets inheriting that hidden preset.

## Local worktree cleanup (disk space)

You may delete **`out/`** (preset build trees) and other paths listed in `.gitignore` that are clearly regenerated outputs (for example Android Gradle/CXX dirs under `platform/android/`, `*.log`, `imgui.ini`). **`external/vcpkg` is not disposable cache**: it is the gitignored Microsoft **vcpkg tool checkout** referenced by `CMAKE_TOOLCHAIN_FILE` in `CMakePresets.json`. Removing it breaks configure until you run `git clone https://github.com/microsoft/vcpkg.git external/vcpkg`, bootstrap `vcpkg.exe`, then `cmake --preset dev` or `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1`. Optional: remove **`vcpkg_installed/`** only when you intend to rerun `tools/bootstrap-deps.ps1` afterward.

For a manual linked worktree under `.worktrees/`, run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/prepare-worktree.ps1` after `git worktree add` and before the first configure. The script verifies the ignored worktree/tool-output roots and links an existing local `external/vcpkg` checkout into the linked worktree instead of downloading packages during CMake configure.

## Install Layout

The root `CMakeLists.txt` installs these artifacts:

| Kind | GNU-layout example | Contents |
| --- | --- | --- |
| Exported targets | `lib/cmake/Mirakanai/` | `MirakanaiConfig.cmake`, `MirakanaiConfigVersion.cmake`, and `MirakanaiTargets.cmake` under the `mirakana::` namespace |
| Libraries / archives | `lib/` | Engine libraries selected by `install(TARGETS ... EXPORT MirakanaiTargets ...)` |
| Executables | `bin/` | Runtime targets, samples, and editor binaries depending on the selected options |
| Public headers | `include/` | Engine module `include/` trees; backend and SDL-facing headers are added only when those targets exist |
| Shared data | `share/Mirakanai/` | `schemas/`, `tools/*.ps1`, sample `games/` fragments, `manifest.json`, `THIRD_PARTY_NOTICES.md`, and related package data |
| Documentation | `share/doc/Mirakanai/` and related doc locations | Filtered copies of `docs/*.md` |

`configure_package_config_file(cmake/MirakanaiConfig.cmake.in ...)` and `write_basic_package_version_file(... COMPATIBILITY ExactVersion)` require the installed package version to match the project `VERSION` exactly.

CPack includes the `ZIP` generator, but day-to-day development should use presets plus `cmake --install` only when an install tree is needed.

## `find_package` After Install

Put the install prefix on `CMAKE_PREFIX_PATH`, or set `Mirakanai_DIR` to `lib/cmake/Mirakanai`.

```cmake
cmake_minimum_required(VERSION 3.30)
project(Consumer LANGUAGES CXX)

find_package(Mirakanai 0.1.0 CONFIG REQUIRED)

add_executable(my_game main.cpp)
target_link_libraries(my_game PRIVATE mirakana::core mirakana::runtime)
```

`MirakanaiConfig.cmake` calls `find_dependency` for SDL3 and optional importer dependencies according to what was enabled when the package was built. See `cmake/MirakanaiConfig.cmake.in`.

## Minimal Exported Target Set

`install(EXPORT MirakanaiTargets)` is limited to static/shared libraries listed in `MK_LIBRARY_TARGETS`. Executables are not part of that export set; they are installed separately with `RUNTIME` destinations.

Always-present core examples:

`MK_ai`, `MK_animation`, `MK_assets`, `MK_audio`, `MK_core`, `MK_math`, `MK_navigation`, `MK_physics`, `MK_platform`, `MK_renderer`, `MK_rhi`, `MK_runtime`, `MK_runtime_host`, `MK_runtime_rhi`, `MK_runtime_scene`, `MK_runtime_scene_rhi`, `MK_rhi_metal`, `MK_rhi_vulkan`, `MK_scene`, `MK_scene_renderer`, `MK_tools`, `MK_ui`, `MK_ui_renderer`

Generated optional targets are added to `MK_LIBRARY_TARGETS` only when the `TARGET` exists, such as `MK_rhi_d3d12`, `MK_platform_sdl3`, or `MK_editor_core`. `EXPORT_NAME` shortens the public package names so consumers reference targets such as `mirakana::core`. Runtime executables are installed outside the export set with `RUNTIME` and Apple `BUNDLE` destinations.

## C++ Modules And `import std`

| `MK_ENABLE_CXX_MODULE_SCANNING` | Effect |
| --- | --- |
| `ON` (default) | Sets `CMAKE_CXX_SCAN_FOR_MODULES=ON`. `MK_apply_common_target_options` enables module dependency scanning per target. |
| `OFF` | Sets `CMAKE_CXX_SCAN_FOR_MODULES=OFF`. Use only for reviewed non-module lanes whose host generator/compiler cannot provide CMake module scanning. |

Default development, C++23 verification, Linux Clang CI, sanitizer, release, and optional vcpkg-backed presets keep scanning `ON`. The `ci-linux-tidy`, `coverage`, and `ci-macos-appleclang` presets keep scanning and CMake-managed `import std` `OFF` because those lanes use clang-tidy without build-generated module maps, GCC coverage, or AppleClang hosts where the current CI contract does not provide official CMake C++ module dependency scanning support. `tools/build-mobile-apple.ps1` also disables scanning and CMake-managed `import std` when configuring the iOS Xcode project, because CMake does not support module dependency scanning with the Xcode generator. Do not add project `FILE_SET CXX_MODULES` sources to those exception lanes without moving them to a supported generator/compiler combination first.

| `MK_ENABLE_IMPORT_STD` | `CMAKE_CXX_COMPILER_IMPORT_STD` contains `23` | Effect |
| --- | --- | --- |
| `ON` (default) | Yes | Sets `CXX_MODULE_STD` on target libraries and enables CMake-managed `import std`. |
| `ON` | No, for example with some Visual Studio generators | Uses headers and project `FILE_SET CXX_MODULES`. Source-level `import std` is limited to what the active toolchain permits. Configure output reports `CMAKE_CXX_COMPILER_IMPORT_STD`. |
| `OFF` | Any | Does not set `CXX_MODULE_STD`. |

See [C++ standard policy](cpp-standard.md) for compiler-specific notes.

## Install Example

After the build tree is ready:

```powershell
cmake --install out/build/dev --prefix _install_prefix
```

The prefix is arbitrary. Build the targets that should be installed before running `cmake --install`; this follows the standard CMake install flow.

---

*Maintain this file alongside `docs/superpowers/plans/2026-05-03-cmake-install-export-and-cxx-modules-audit-v1.md`.*



