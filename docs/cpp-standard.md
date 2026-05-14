# C++ Standard Policy

## Current Default

Mirakanai requires C++23 through `MK_CXX_STANDARD=23`.

Lower standards are rejected during CMake configure. This repository is still pre-release and intentionally does not preserve backward compatibility, so generated games, engine APIs, schemas, and AI guidance should all assume C++23.

## CMake Policy

Mirakanai targets use:

- `target_compile_features(... PUBLIC cxx_std_${MK_CXX_STANDARD})`
- `CXX_EXTENSIONS OFF`
- `CXX_STANDARD_REQUIRED ON`
- `CXX_SCAN_FOR_MODULES` driven by `MK_ENABLE_CXX_MODULE_SCANNING` (default ON; see [Building](building.md))

The project requires CMake 3.30 or newer so module scanning policy and CMake-managed `import std;` detection are not configured below their official support floor.

MSVC targets also use `/permissive-` and `/EHsc`.

The formatter uses `.clang-format` `Standard: Latest` so the locally supported official LLVM formatter parses the newest C++ grammar available to that formatter. Switch this to `Standard: c++23` once the supported local clang-format accepts it.

## MSVC Standard Mode

MSVC builds use `MK_MSVC_CXX23_STANDARD_OPTION=/std:c++23preview` by default.

This overrides CMake's current MSVC C++23 mapping from `/std:c++latest` to the selected C++23-only option. The goal is to use the ISO C++23 mode and avoid accidentally accepting later working-draft features. Today the selected option is `/std:c++23preview`. When MSVC and CMake expose a stable `/std:c++23` path, change `MK_MSVC_CXX23_STANDARD_OPTION` in the cache default and presets to `/std:c++23`; validation checks derive the generated-project expectation from that cache value.

Visual Studio generators may encode the selected mode either as a raw `/std:c++23preview` or `/std:c++23` option in `AdditionalOptions`, or as a `.vcxproj` `<LanguageStandard>` value such as `stdcpp23preview` / `stdcpp23`. The validation contract accepts those C++23-only representations and continues to reject `stdcpplatest`, `stdcpp26`, or `/std:c++latest`.

## Modules And `import std`

Project C++ modules are allowed and should be added with CMake `FILE_SET CXX_MODULES`.

`MK_ENABLE_CXX_MODULE_SCANNING=ON` is enabled by default so module dependency scanning is available for targets and source files.

`MK_ENABLE_IMPORT_STD=ON` is also enabled by default, but CMake-managed `import std;` is only activated when the active generator/toolchain reports C++23 standard-library module support through `CMAKE_CXX_COMPILER_IMPORT_STD`.

Current local CMake `3.31.6-msvc6` with the Visual Studio generator does not provide CMake-managed `import std;` support. The policy is therefore:

- Use project modules through `FILE_SET CXX_MODULES`.
- Use `import std;` first in targets/configurations where CMake reports support.
- Keep public installed SDK headers available until module export/install support is explicitly designed.
- Do not use `/std:c++latest` to unlock standard-library modules unless a task explicitly accepts the C++26 working-draft exposure risk.

## Verification

Run the default validation path:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Run explicit C++23 verification:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/evaluate-cpp23.ps1
```

For Release/package verification:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/evaluate-cpp23.ps1 -Release
```

For the optional SDL3/Dear ImGui editor path:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/evaluate-cpp23.ps1 -Gui
```

For full local confidence on Windows:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/evaluate-cpp23.ps1 -Release -Gui
```

## Local Result

Evaluated on 2026-04-26 with:

- CMake `3.31.6-msvc6`
- MSBuild `17.14.23`
- MSVC `19.44.35222.0`
- Windows SDK `10.0.26100.0`

Results:

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: passed, 12/12 default tests.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/evaluate-cpp23.ps1 -Release -Gui`: passed, 12/12 Debug tests, 12/12 Release tests, generated `Mirakanai-0.1.0-Windows-AMD64.zip`, and passed 13/13 SDL3/Dear ImGui GUI tests.
- CMake generated Visual Studio projects with the configured `MK_MSVC_CXX23_STANDARD_OPTION` in target options, or an equivalent C++23-only Visual Studio `LanguageStandard` representation, and no `stdcpplatest` language standard entries in the checked default build.
- MSVC targets explicitly use `/EHsc` so C++ exception unwinding stays enabled.

## Recommendation

Keep C++23 as the required Mirakanai baseline.

Outstanding work is platform breadth, not a migration blocker:

- Add Linux GCC C++23 build and sanitizer coverage.
- Add Clang/libc++ or AppleClang evaluation for macOS/iOS targets.
- Add Android NDK evaluation for mobile runtime targets.
- Track MSVC/CMake support for a stable strict C++23 flag and switch `MK_MSVC_CXX23_STANDARD_OPTION` from `/std:c++23preview` to `/std:c++23` when available through the supported generator path.
- Track CMake `import std;` support for Visual Studio generators and enable standard-library module use in default targets when CMake reports support.

## Source Notes

- CMake exposes `cxx_std_23` as a compile-feature meta level, but it reflects claimed compiler support and does not prove complete language or standard-library conformance.
- MSVC documents `/std:c++23preview` as the C++23 preview mode and `/std:c++latest` as including currently implemented future-draft and in-progress features.
- CMake module scanning should use `FILE_SET CXX_MODULES`; CMake-managed `import std;` depends on `CMAKE_CXX_COMPILER_IMPORT_STD` and generator/toolchain support.
- GCC and Clang document partial or experimental C++23 support depending on compiler and standard-library version.
