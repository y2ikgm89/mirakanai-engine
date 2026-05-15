# Runtime Package Index Discovery v1 (2026-05-15)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `runtime-package-index-discovery-v1`

**Goal:** Add a host-independent Runtime Resource v2 discovery contract that finds reviewed cooked package index candidates through `IFileSystem` before any package load, mount, streaming, or renderer/RHI work.

**Architecture:** `MK_runtime` will expose deterministic value/result types plus `discover_runtime_package_indexes_v2`. The implementation reads only `IFileSystem::is_directory(root)` and `IFileSystem::list_files(root)`, filters valid `.geindex` candidates, derives a stable package label, preserves a caller-provided optional content root for later explicit load requests, and returns diagnostics for invalid discovery descriptors or VFS scan failures without mutating runtime state.

**Tech Stack:** C++23, `MK_runtime`, first-party `IFileSystem`, CTest.

---

## Context

The active master-plan gap is `runtime-resource-v2`. Resident mount sets, catalog cache refresh, safe resident mount/replace/unmount execution, and reviewed eviction planning are already implemented. The remaining follow-up claims include disk/VFS package load/mount execution beyond reviewed candidate discovery, package-streaming breadth, hot reload productization, renderer/RHI ownership, upload/staging integration, arbitrary eviction, and allocator/GPU budget enforcement.

This slice chooses the smallest disk/VFS step: discover candidate package indexes from a caller-owned virtual filesystem root. It intentionally stops before package loading or mutation so later host/editor code can review and select candidates before calling existing `load_runtime_asset_package` and resident mount/replace APIs.

Official practice check:

- CMake/CTest: Context7 resolved official Kitware CMake docs (`/kitware/cmake`), confirming the repository's existing pattern of `add_executable(...)`, `target_link_libraries(... PRIVATE ...)`, `target_include_directories(... PRIVATE ...)`, and `add_test(NAME ... COMMAND ...)`.
- The runtime API stays value/result oriented and RAII-friendly: no raw/native handles, no background work, no ownership leaks, and no hidden mutation.
- The project is greenfield, so the v2 discovery contract is added directly without compatibility aliases or migration shims.

## Constraints

- Keep discovery in `MK_runtime` and independent from editor, SDL3, renderer/RHI, upload/staging, package streaming execution, native filesystem handles, and background workers.
- Use `IFileSystem` only. `RootedFileSystem` and `MemoryFileSystem` remain the host/VFS adapters.
- Do not call `load_runtime_asset_package`, mount discovered packages, refresh resident catalogs, or mutate `RuntimeAssetPackageStore`.
- Return deterministic sorted candidates from the filesystem's listed files.
- Reject empty roots, unsafe roots, unsafe content roots, missing/non-directory roots, and root scan failures with diagnostics.
- Preserve `content_root` exactly after trailing-slash normalization; do not infer it from a package index path.
- Filter invalid candidate paths rather than throwing for ordinary non-package files.
- Keep arbitrary recursive policy, LRU/hot reload policy, GPU/allocator budget enforcement, renderer/RHI resource ownership, and upload/staging integration as explicit follow-ups.

## Implementation Checklist

- [x] Add failing discovery tests in `tests/unit/runtime_package_index_discovery_tests.cpp` for sorted `.geindex` candidates, invalid descriptor diagnostics, missing root diagnostics, ignored non-package files, and no runtime mutation.
- [x] Register `MK_runtime_package_index_discovery_tests` in `CMakeLists.txt` using the existing target-based unit-test pattern.
- [x] Add public value types and `discover_runtime_package_indexes_v2` to `engine/runtime/include/mirakana/runtime/resource_runtime.hpp`.
- [x] Implement discovery in `engine/runtime/src/resource_runtime.cpp` using only `IFileSystem::is_directory` and `IFileSystem::list_files`.
- [x] Run the focused RED/GREEN build and CTest loop for `MK_runtime_package_index_discovery_tests`.
- [x] Reconcile docs, plan registry, manifest fragments plus composed manifest, and agent/static checks for the new AI-operable contract.
- [x] Run the slice-closing validation gate.

## Done When

- A caller can discover deterministic `.geindex` package candidates under a reviewed filesystem root.
- Each candidate includes `package_index_path`, `content_root`, and a stable label suitable for later reviewed load/mount selection.
- Empty roots and missing/non-directory roots report deterministic diagnostics.
- Discovery does not load packages, mount resident packages, refresh catalogs, mutate stores, spawn background work, or touch renderer/RHI/native handles.
- Validation evidence is recorded below.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `cmake --build --preset dev --target MK_runtime_package_index_discovery_tests` | RED failed as expected, then Passed | RED failed before `RuntimePackageIndexDiscovery*` types and `discover_runtime_package_indexes_v2` existed; GREEN passed through the normalized build wrapper after implementation. |
| `ctest --preset dev --output-on-failure -R MK_runtime_package_index_discovery_tests` | Passed | Focused discovery suite passed. |
| `cmake --build --preset dev --target MK_runtime_package_index_discovery_tests MK_runtime_resource_resident_cache_tests MK_runtime_resource_resident_unmount_tests MK_runtime_resource_resident_replace_tests MK_runtime_package_streaming_resident_mount_tests` | Passed | Focused discovery and resident regression targets built; MSBuild emitted existing intermediate-directory sharing warnings. |
| `ctest --preset dev --output-on-failure -R "^(MK_runtime_package_index_discovery_tests\|MK_runtime_resource_resident_cache_tests\|MK_runtime_resource_resident_unmount_tests\|MK_runtime_resource_resident_replace_tests\|MK_runtime_package_streaming_resident_mount_tests)$"` | Passed | 5/5 focused and resident regression suites passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed | Public API boundary drift check passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed | Format/LF/UTF-8 check passed after `tools/format.ps1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/resource_runtime.cpp,tests/unit/runtime_package_index_discovery_tests.cpp` | Passed | Focused tidy passed for 2 files. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed | Manifest composition and JSON contract checks passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed | Agent surface budget and parity checks passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | AI integration needles passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Slice-closing validation passed; diagnostic-only Metal/Apple host gates remain blocked on this Windows host as expected. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Passed | Required standalone pre-commit build passed; MSBuild emitted existing intermediate-directory sharing warnings. |
