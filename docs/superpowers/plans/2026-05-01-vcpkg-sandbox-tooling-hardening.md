# vcpkg Sandbox Tooling Hardening (2026-05-01)

## Goal

Make optional vcpkg-backed Windows build lanes use a clean, official vcpkg/CMake contract that does not run vcpkg dependency restore from CMake configure and is less sensitive to sandbox process-handle restrictions or user-global cache state.

## Context

Recent desktop runtime and GUI validation repeatedly hit a sandbox-only vcpkg/7zip blocker reported as `CreateFileW stdin failed with 5`. Deeper reproduction showed 7zip extraction was only one symptom: forcing existing tools still failed inside vcpkg's CMake-variable probing path. The architectural issue was that vcpkg could be launched implicitly during CMake configure, where sandbox process-handle restrictions are hardest to diagnose and easiest to repeat across build lanes.

Official vcpkg documentation provides environment variables for downloads and binary caching, and CMake integration variables for manifest install behavior:

- `VCPKG_DOWNLOADS`
- `VCPKG_DEFAULT_BINARY_CACHE`
- `VCPKG_BINARY_SOURCES`
- `VCPKG_DISABLE_METRICS`
- `VCPKG_MANIFEST_INSTALL`
- `VCPKG_INSTALLED_DIR`

## Constraints

- Keep optional C++ dependencies in existing vcpkg manifest features.
- Do not change the pinned `builtin-baseline`.
- Do not add third-party dependencies.
- Keep generated/cache output under `out/`.
- Keep configure/build/package wrappers non-interactive and free of configure-time dependency downloads.

## Design

Add a shared PowerShell helper in `tools/common.ps1` that creates `out/vcpkg/downloads` and `out/vcpkg/binary-cache`, then sets the official vcpkg process environment variables before optional vcpkg lanes run. Existing vcpkg-downloaded tool directories are added to `Path` when present so explicit bootstrap can reuse them.

Keep `vcpkg install` explicit and centralized in `tools/bootstrap-deps.ps1`. It installs the `desktop-runtime`, `desktop-gui`, and `asset-importers` manifest features into the repository root `vcpkg_installed` tree.

Set every optional vcpkg-backed CMake preset to `VCPKG_MANIFEST_INSTALL=OFF` and `VCPKG_INSTALLED_DIR=${sourceDir}/vcpkg_installed`. Configure steps therefore use the official vcpkg CMake integration for `find_package()` search paths but do not run `vcpkg install`, download archives, or extract vcpkg tools during configure.

Apply the shared vcpkg environment helper to:

- `tools/bootstrap-deps.ps1`
- `tools/build-gui.ps1`
- `tools/validate-desktop-game-runtime.ps1`
- `tools/package-desktop-runtime.ps1`
- `tools/build-asset-importers.ps1`
- `tools/evaluate-cpp23.ps1` when `-Gui` is selected

## Done When

- `tools/check-vcpkg-environment.ps1` proves the vcpkg env values, cache directories, optional script wiring, child-process `Path` propagation, and no synthetic stdin policy.
- `tools/check-dependency-policy.ps1` proves optional vcpkg-backed CMake presets disable configure-time manifest install, use the root `vcpkg_installed` tree, and leave manifest feature selection in `bootstrap-deps`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1` passes inside the sandbox without invoking configure-time vcpkg restore.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes, or records a concrete host/toolchain blocker.
- Dependency/testing docs and plan registry describe the new wrapper policy.

## Evidence

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-vcpkg-environment.ps1` failed because `Set-GameEngineVcpkgEnvironment` did not exist.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1` failed because vcpkg-backed CMake presets did not set `VCPKG_MANIFEST_INSTALL=OFF`.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-vcpkg-environment.ps1` failed while it still required `Invoke-CheckedCommand` to synthesize stdin handles.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-vcpkg-environment.ps1` passed after adding the helper, path propagation, vcpkg tool discovery, and no-synthetic-stdin policy.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1` passed after moving optional CMake presets to explicit bootstrap consumption.
- BLOCKER CLASSIFIED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1` still failed inside the sandbox with `CreateFileW stdin failed with 5` because it intentionally runs vcpkg. The approved unrestricted rerun completed the explicit dependency bootstrap and populated the root `vcpkg_installed` tree.
- GREEN: `cmake --preset desktop-runtime-release --fresh` passed with `VCPKG_MANIFEST_INSTALL=OFF` and no unused `VCPKG_MANIFEST_FEATURES` warning.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1` passed inside the sandbox; configure completed without vcpkg restore, build/tests/install validation ran, and CPack generated the desktop runtime ZIP.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` passed with 14/14 CTest tests.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` passed with 41/41 CTest tests.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-asset-importers.ps1` passed, and a follow-up fresh `cmake --preset asset-importers --fresh` confirmed the root `vcpkg_installed` tree is used.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with 28/28 default CTest tests. Diagnostic-only host gates remained Metal `metal`/`metallib` missing, Apple packaging requiring macOS/Xcode, Android release signing not configured, Android device smoke not connected, and strict tidy compile database availability.
