# CI PR Check Root Cause Hardening v1 (2026-05-13)

**Plan ID:** `ci-pr-check-root-cause-hardening-v1`
**Gap:** `full-repository-quality-gate`
**Parent:** [2026-05-11-phase4-full-repository-quality-gate-ci-analyzer-expansion-v1.md](2026-05-11-phase4-full-repository-quality-gate-ci-analyzer-expansion-v1.md)
**Status:** Completed

## Goal

Make PR validation green by fixing the root causes behind the failed GitHub checks instead of masking individual jobs.

## Context

PR #5 failed across Windows, Linux, Linux sanitizer, static analysis, macOS, and iOS. The failures were caused by CI environment drift from the repository contract, host line-ending/tool environment differences, C++23 standard-library implementation gaps on hosted Clang/AppleClang, clang-tidy module-map timing, CMake bundle install requirements on Apple generators, Xcode generator C++ module-scanning incompatibility, a Metal runtime encoder access-control bug, and a real Linux/macOS native watcher API bug.

## Constraints

- Keep PowerShell 7 wrappers as the repository command surface.
- Keep vcpkg installation outside CMake configure; CI may restore the gitignored `external/vcpkg` checkout before `tools/bootstrap-deps.ps1`.
- Use official CMake C++ module scanning support: supported generator/compiler combinations keep scanning on; unsupported CI host lanes must be explicit exceptions.
- Preserve greenfield clean-break policy; do not add compatibility shims for the incorrect static native watcher API.

## Done When

- Windows CI restores a pinned vcpkg checkout before dependency bootstrap.
- Linux build/sanitizer CI use supported Ninja + Clang presets for C++ module scanning, while static-analysis uses an explicit clang-tidy preset that does not depend on build-generated module maps.
- macOS CI uses an explicit AppleClang preset with module scanning/import-std disabled until the host provides an officially supported scanning path.
- Linux coverage uses a separate preset and initializes the lcov summary input correctly.
- Linux/macOS native watcher `active()` is an instance `const noexcept` API matching Windows.
- C++ code avoids C++23 library calls not present on hosted Clang/AppleClang standard libraries when C++20 alternatives are sufficient.
- Apple bundle configure paths include CMake `BUNDLE DESTINATION` for runtime executable installs.
- Apple iOS Xcode configure paths explicitly disable CMake C++ module scanning and CMake-managed `import std`.
- Metal runtime render encoder creation has complete friend declarations for texture and drawable paths.
- Windows CRLF checkouts and macOS/iOS missing Windows-only environment variables do not crash validation scripts or source registry key checks.
- Local validation covers the updated scripts, docs, presets, and native watcher compile contract.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `cmake --build --preset dev --target MK_core_tests` after RED static assertions | PASS | Static assertions failed first for the incorrect static Linux/macOS watcher API, then passed after the clean instance API fix. |
| `ctest --preset dev --output-on-failure -R MK_core_tests` | PASS | Targeted public API and file watcher contract test passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-coverage-thresholds.ps1` | PASS | Confirms `tools/check-coverage.ps1` initializes `$currentInfo` before lcov filtering/summary. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1` | PASS | CI workflow static contract matches pinned vcpkg checkout, Linux/macOS presets, coverage, and static-analysis artifacts. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | JSON/manifests and workflow needles are synchronized. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-cpp-standard-policy.ps1` | PASS | C++23/module-scanning policy recognizes `ci-linux-clang`, `ci-linux-tidy`, `coverage`, and `ci-macos-appleclang`. |
| `$env:LOCALAPPDATA=$null; $env:ProgramFiles=$null; pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-mobile-packaging.ps1` | PASS | Diagnostic-only mobile validation no longer crashes on non-Windows hosts with missing Windows-only environment variables. |
| `rg -n "^[^/\"]*std::ranges::(contains\|iota)" engine editor tests games examples` | PASS | No unsupported hosted-library calls remain in compiled source/test trees; `std::iota` uses are documented with targeted `NOLINT` because hosted Clang/AppleClang CI lacks `std::ranges::iota`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | Agent-facing workflow/skill contract stayed synchronized. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Header API change does not violate public boundary policy. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/platform/src/linux_file_watcher.cpp,engine/platform/src/macos_file_watcher.cpp,tests/unit/core_tests.cpp -MaxFiles 3` | PASS | Targeted tidy completed; existing repository warning profile remains warning-only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Strict -Files editor/core/src/material_graph_authoring.cpp,editor/core/src/material_authoring.cpp,engine/assets/src/sprite_atlas_packing.cpp,engine/navigation/src/local_avoidance.cpp,engine/platform/src/mobile.cpp,engine/runtime_scene_rhi/src/runtime_scene_rhi.cpp,engine/scene_renderer/src/scene_renderer.cpp,engine/runtime/src/runtime_diagnostics.cpp,engine/runtime/src/asset_runtime.cpp,engine/tools/asset/ui_atlas_tool.cpp,engine/tools/asset/tilemap_tool.cpp,engine/rhi/vulkan/src/vulkan_backend.cpp,tests/unit/editor_core_tests.cpp -MaxFiles 13` | PASS | Targeted tidy covered hosted-library fallback edits; existing repository warning profile remains warning-only. |
| `cmake --build --preset dev --target mirakana_rhi_vulkan` | PASS | Rebuilt the edited Vulkan backend after removing a non-ASCII comment that triggered MSVC C4819. |
| `cmake --build --preset dev --target mirakana_runtime MK_runtime_tests MK_editor_core` | PASS | Rebuilt runtime parsing, Linux-tidy include fix, and editor backend direct include changes. |
| `cmake --build --preset dev --target MK_tools_scene MK_tools_tests` | PASS | Rebuilt the scene tools after replacing remaining floating-point `std::from_chars` parsing with classic-locale parsing. |
| `ctest --preset dev --output-on-failure -R "MK_runtime_tests\|MK_tools_tests"` | PASS | Targeted runtime and tools tests passed after hosted CI portability fixes. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Strict -Files engine/runtime/src/asset_runtime.cpp,tests/unit/runtime_tests.cpp,editor/core/src/render_backend.cpp -MaxFiles 3` | PASS | Targeted tidy covered macOS libc++ float parsing and Linux clang direct-include fixes; existing repository warning profile remains warning-only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Strict -Files engine/tools/scene/physics_collision_package_tool.cpp -MaxFiles 1` | PASS | Targeted tidy covered the remaining first-party float parsing fallback. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full Windows validation, build, and 51 CTest tests passed; Apple/Metal diagnostics remained host-gated on Windows. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-cpp-standard-policy.ps1` | PASS | Static policy now locks Apple mobile Xcode configure to `MK_ENABLE_CXX_MODULE_SCANNING=OFF` and `MK_ENABLE_IMPORT_STD=OFF`. |
| `cmake --build --preset dev --target MK_rhi_metal` | PASS | Rebuilt the Metal target after adding the missing drawable render-encoder friend declaration. |
