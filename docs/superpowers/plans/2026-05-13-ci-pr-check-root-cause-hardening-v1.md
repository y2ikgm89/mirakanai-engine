# CI PR Check Root Cause Hardening v1 (2026-05-13)

**Plan ID:** `ci-pr-check-root-cause-hardening-v1`
**Gap:** `full-repository-quality-gate`
**Parent:** [2026-05-11-phase4-full-repository-quality-gate-ci-analyzer-expansion-v1.md](2026-05-11-phase4-full-repository-quality-gate-ci-analyzer-expansion-v1.md)
**Status:** Completed

## Goal

Make PR validation green by fixing the root causes behind the failed GitHub checks instead of masking individual jobs.

## Context

PR #5 failed across Windows, Linux, Linux sanitizer, static analysis, and macOS. The failures were caused by CI environment drift from the repository contract plus a real Linux/macOS native watcher API bug.

## Constraints

- Keep PowerShell 7 wrappers as the repository command surface.
- Keep vcpkg installation outside CMake configure; CI may restore the gitignored `external/vcpkg` checkout before `tools/bootstrap-deps.ps1`.
- Use official CMake C++ module scanning support: supported generator/compiler combinations keep scanning on; unsupported CI host lanes must be explicit exceptions.
- Preserve greenfield clean-break policy; do not add compatibility shims for the incorrect static native watcher API.

## Done When

- Windows CI restores a pinned vcpkg checkout before dependency bootstrap.
- Linux/static-analysis CI use a supported Ninja + Clang preset for C++ module scanning.
- macOS CI uses an explicit AppleClang preset with module scanning/import-std disabled until the host provides an officially supported scanning path.
- Linux coverage uses a separate preset and initializes the lcov summary input correctly.
- Linux/macOS native watcher `active()` is an instance `const noexcept` API matching Windows.
- Local validation covers the updated scripts, docs, presets, and native watcher compile contract.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `cmake --build --preset dev --target MK_core_tests` after RED static assertions | PASS | Static assertions failed first for the incorrect static Linux/macOS watcher API, then passed after the clean instance API fix. |
| `ctest --preset dev --output-on-failure -R MK_core_tests` | PASS | Targeted public API and file watcher contract test passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-coverage-thresholds.ps1` | PASS | Confirms `tools/check-coverage.ps1` initializes `$currentInfo` before lcov filtering/summary. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1` | PASS | CI workflow static contract matches pinned vcpkg checkout, Linux/macOS presets, coverage, and static-analysis artifacts. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | JSON/manifests and workflow needles are synchronized. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-cpp-standard-policy.ps1` | PASS | C++23/module-scanning policy recognizes `ci-linux-clang`, `coverage`, and `ci-macos-appleclang`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | Agent-facing workflow/skill contract stayed synchronized. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Header API change does not violate public boundary policy. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/platform/src/linux_file_watcher.cpp,engine/platform/src/macos_file_watcher.cpp,tests/unit/core_tests.cpp -MaxFiles 3` | PASS | Targeted tidy completed; existing repository warning profile remains warning-only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full Windows validation, build, and 51 CTest tests passed; Apple/Metal diagnostics remained host-gated on Windows. |
