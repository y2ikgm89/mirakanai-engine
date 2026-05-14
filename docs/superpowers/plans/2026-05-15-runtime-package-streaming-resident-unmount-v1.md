# Runtime Package Streaming Resident Unmount v1 (2026-05-15)

**Plan ID:** `runtime-package-streaming-resident-unmount-v1`

## Goal

Add a reviewed package-streaming safe-point path that removes one existing resident package mount through the already validated resident unmount commit, preserving the live mount set and catalog cache on every preflight or commit failure.

## Context

`runtime-resource-v2` now has explicit resident mount, catalog cache, streaming resident mount, resident unmount/cache refresh, resident replacement commit, and streaming resident replacement slices. The missing symmetric package-streaming execution surface is resident unmount: operators can mount and replace through selected safe points, but cannot yet remove a selected resident mount through the same AI-operable package-streaming contract.

Official practice check:

- This slice adds no dependency, SDK, CMake, vcpkg, renderer/RHI, platform, or background worker behavior.
- The design stays value/result oriented and RAII-friendly, aligned with [C++ Core Guidelines R.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#r1-manage-resources-automatically-using-resource-handles-and-raii): mutation is explicit, resources stay behind first-party handles, and failure paths preserve owned state.
- The project is greenfield: add the clean v2 execution surface directly, with no compatibility alias or migration shim.

## Constraints

- Keep `MK_runtime` independent from renderer/RHI native handles, editor code, arbitrary shell execution, filesystem discovery, and background streaming.
- Use `commit_runtime_resident_package_unmount_v2` as the mutation primitive; do not duplicate lower-level unmount/cache commit behavior.
- Validate descriptor mode, runtime scene preflight, explicit nonzero/existing mount id, remaining-package count hints, required preload survival, allowed resident resource kinds, budget, and catalog refresh before reporting success.
- Preserve live mount set and cached catalog on invalid descriptor, missing validation preflight, invalid/missing mount id, residency hint failure, budget failure, and catalog build failure.
- Keep ready claims narrow: no arbitrary eviction, enforced allocator/GPU budgets, disk/VFS mount discovery, renderer/RHI ownership, upload/staging package integration, broad/background streaming, or hot reload productization.

## Implementation Checklist

- [x] Add failing tests in `tests/unit/runtime_package_streaming_resident_mount_tests.cpp` for selected resident package unmount success, invalid/missing id preservation, and residency-hint preservation.
- [x] Add a package-streaming resident unmount result path in `engine/runtime/include/mirakana/runtime/package_streaming.hpp`.
- [x] Implement the unmount execution in `engine/runtime/src/package_streaming.cpp` by validating the descriptor, mount id, projected remaining catalog hints, and then calling `commit_runtime_resident_package_unmount_v2`.
- [x] Run focused build/test for `MK_runtime_package_streaming_resident_mount_tests`.
- [x] Reconcile current capabilities, roadmap/plan registry, manifest fragments + composed manifest, and static checks for the new AI-operable contract.
- [x] Run the slice-closing validation gate.

## Done When

- Selected package streaming can remove one existing resident mount at a safe point and refresh `RuntimeResidentCatalogCacheV2` through the existing unmount commit.
- Required preload assets and resident resource kind hints are checked against the projected remaining resident catalog before live mutation.
- Deterministic result rows and diagnostics identify unmount, residency hint, budget, and catalog-refresh failures.
- Validation evidence is recorded below.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/prepare-worktree.ps1` | Passed | Linked worktree dependency junctions and local build roots were prepared. |
| `cmake --build --preset dev --target MK_runtime_package_streaming_resident_mount_tests` | Passed | First run failed as expected before adding the API/status; focused GREEN build passed after implementation. |
| `ctest --preset dev --output-on-failure -R MK_runtime_package_streaming_resident_mount_tests` | Passed | Focused package streaming resident mount/replace/unmount suite passed. |
| `cmake --build --preset dev --target MK_runtime_resource_resident_unmount_tests MK_runtime_resource_resident_cache_tests MK_runtime_resource_resident_replace_tests` | Passed | Resident unmount/cache/replace regression targets passed. |
| `ctest --preset dev --output-on-failure -R "^(MK_runtime_package_streaming_resident_mount_tests\|MK_runtime_resource_resident_unmount_tests\|MK_runtime_resource_resident_cache_tests\|MK_runtime_resource_resident_replace_tests)$"` | Passed | Four runtime resource resident regression suites passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed | Public runtime API boundary stayed clean. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed | Passed after `tools/format.ps1` normalized touched C++ sources, and again after static guard updates. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/package_streaming.cpp,tests/unit/runtime_package_streaming_resident_mount_tests.cpp` | Passed | Focused tidy passed; one clang frontend warning summary was suppressed by the repository wrapper. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed | Manifest compose and JSON contract checks passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed | Agent surface configuration stayed within policy. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | AI integration drift check passed with generated dry-run game scaffolds. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Full validation passed; Metal/Apple host checks remained diagnostic or host-gated on Windows as expected. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Passed | Standalone commit-gate build passed after full validation. |
