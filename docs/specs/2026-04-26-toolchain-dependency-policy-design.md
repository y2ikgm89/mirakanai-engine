# Toolchain Dependency Policy Design

## Goal

Keep GameEngine dependency choices suitable for a production-grade engine: official sources only, minimal default dependencies, reproducible optional dependencies, and automated checks that make policy drift visible.

## Design

- The default build stays third-party-free.
- Project-wide build settings live in checked-in `CMakePresets.json`; local developer overrides live in ignored `CMakeUserPresets.json`.
- `tools/check-toolchain.ps1` is the repository toolchain preflight. It reports the CMake, CTest, optional CPack, Visual Studio, and MSBuild paths used by wrappers and enforces CMake/CTest 3.30+.
- On Windows, direct CMake usage assumes Visual Studio Developer PowerShell/Command Prompt or official CMake 3.30+ on `PATH`. `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1 -RequireDirectCMake` enforces that direct-command precondition. Repository wrappers may invoke the resolved full path to CMake so CI and agents are not dependent on a human shell profile.
- Optional GUI/editor dependencies stay isolated under the `desktop-gui` vcpkg manifest feature.
- `vcpkg.json` pins the official Microsoft vcpkg registry with `builtin-baseline` so SDL3, Dear ImGui, and their transitive helper ports resolve reproducibly.
- Dependency changes must update `docs/dependencies.md` and `THIRD_PARTY_NOTICES.md` in the same task.
- `tools/check-dependency-policy.ps1` performs static validation before CMake work starts:
  - `vcpkg.json` references the official schema.
  - `builtin-baseline` exists and is a 40-character commit hash.
  - default dependencies remain empty.
  - `desktop-gui` declares `sdl3` and `imgui`.
  - Dear ImGui keeps docking, SDL3 platform, and SDL3 renderer bindings enabled.
  - third-party notices and dependency docs include the required records.
  - `bootstrap-deps` owns vcpkg manifest feature installation, while optional CMake presets disable configure-time manifest install and consume the root `vcpkg_installed` tree.

## Update Flow

1. Update the official `external/vcpkg` checkout from `https://github.com/microsoft/vcpkg`.
2. Confirm selected upstream versions and licenses from official package metadata.
3. Update `vcpkg.json` `builtin-baseline`.
4. Update `docs/dependencies.md` and `THIRD_PARTY_NOTICES.md`.
5. Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

## Non-Goals

- Do not add runtime third-party dependencies to the default build.
- Do not pin unofficial forks or mirrors.
- Do not upgrade the project C++ standard or CMake minimum just because a newer version exists; raise minimums only when the engine needs a feature and CI/toolchain support is verified.