# Runtime Package Candidate Resident Mount v1 (2026-05-15)

**Plan ID:** `runtime-package-candidate-resident-mount-v1`
**Status:** Completed child slice under `production-completion-master-plan-v1` / `runtime-resource-v2`
**Goal:** Add a host-independent reviewed disk/VFS package mount execution contract that loads one selected `.geindex` discovery candidate and commits it into an explicit resident mount set plus catalog cache without package-streaming descriptors, background workers, hot reload, renderer/RHI ownership, upload/staging, or native handles.

## Context

`runtime-resource-v2` now has reviewed `.geindex` discovery through `discover_runtime_package_indexes_v2` and reviewed selected candidate loading through `load_runtime_package_candidate_v2`. Existing resident mount/cache primitives can mount already-loaded package values, and package-streaming safe-point helpers can mount already-loaded packages through a streaming descriptor. The next narrow disk/VFS step is a reviewed candidate-to-resident-mount helper that composes candidate load with the resident mount/cache path while preserving copy-then-commit behavior.

## Official Practice Check

- CMake: continue the repository's target-scoped CMake/CTest pattern for unit-test executables.
- C++/API: follow `docs/cpp-style.md`, C++23 value-type result contracts, explicit statuses, no compatibility aliases, and no broad ready claims.

## Constraints

- Keep the helper in `MK_runtime` and independent from editor, SDL3, renderer/RHI, upload/staging, native filesystem handles, package-streaming descriptors, background workers, and hot reload controllers.
- Validate the explicit resident mount id before reading candidate package files, so invalid or duplicate mount ids do not perform disk/VFS reads.
- Use `load_runtime_package_candidate_v2` for candidate validation, package reads, low-level loader failures, invalid index text, and read exceptions.
- Refresh `RuntimeResidentCatalogCacheV2` only on projected state first; commit the mount set and catalog cache together only after mount, budget, and catalog refresh preflight pass.
- Preserve the previous mount set and cached catalog on invalid mount id, duplicate mount id, invalid candidate, load failure, read failure, budget failure, or catalog refresh failure.
- Do not add arbitrary eviction, broad/background streaming, package-streaming breadth, hot reload productization, renderer/RHI ownership, upload/staging integration, allocator/GPU budget enforcement, or 2D/3D vertical-slice ready claims.

## Tasks

- [x] Add RED tests in `tests/unit/runtime_package_candidate_resident_mount_tests.cpp` for successful candidate mount/cache refresh, invalid and duplicate mount ids before reads, candidate/load/read failures without mutation, and projected budget failure rollback.
- [x] Add the `MK_runtime_package_candidate_resident_mount_tests` CMake target using target-scoped wiring.
- [x] Add public status/result types and `commit_runtime_package_candidate_resident_mount_v2` to `engine/runtime/include/mirakana/runtime/resource_runtime.hpp`.
- [x] Implement mount-id preflight, candidate load composition, projected mount/cache refresh, and copy-then-commit behavior in `engine/runtime/src/resource_runtime.cpp`.
- [x] Update runtime/resource docs, roadmap/current capabilities, plan registry/master plan, and manifest fragments/composed manifest.
- [x] Run focused build/test/static checks, then full `validate.ps1` and `build.ps1`.

## Done When

- A reviewed selected `.geindex` candidate can be loaded from disk/VFS and mounted into a resident mount set with catalog cache refresh as one host-independent commit helper.
- Invalid or duplicate mount ids fail before any filesystem read.
- Candidate validation failures, package load failures, invalid package index text, read exceptions, projected budget failures, and catalog refresh failures leave the live mount set and cached catalog unchanged.
- The helper reports typed status and sub-results for candidate load, resident mount, and resident catalog refresh.
- Current-truth docs and `engine/agent/manifest.json` describe this as candidate resident mount execution only, with broader streaming/hot reload/RHI/upload/eviction/budget gaps still follow-up claims.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `cmake --build --preset dev --target MK_runtime_package_candidate_resident_mount_tests` | PASS | RED failed before implementation on missing candidate resident mount API; GREEN passed after implementation. |
| `ctest --preset dev --output-on-failure -R MK_runtime_package_candidate_resident_mount_tests` | PASS | Focused candidate resident mount suite passed. |
| `cmake --build --preset dev --target MK_runtime_package_candidate_resident_mount_tests MK_runtime_package_candidate_load_tests MK_runtime_package_index_discovery_tests MK_runtime_package_streaming_resident_mount_tests MK_runtime_resource_resident_cache_tests MK_runtime_resource_resident_unmount_tests MK_runtime_resource_resident_replace_tests MK_runtime_tests` | PASS | Focused runtime regression target set passed. |
| `ctest --preset dev --output-on-failure -R "MK_runtime_package_candidate_resident_mount_tests|MK_runtime_package_candidate_load_tests|MK_runtime_package_index_discovery_tests|MK_runtime_package_streaming_resident_mount_tests|MK_runtime_resource_resident_cache_tests|MK_runtime_resource_resident_unmount_tests|MK_runtime_resource_resident_replace_tests|MK_runtime_tests"` | PASS | Focused runtime regression suite passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public runtime header boundary check passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Formatting/text checks passed after normalizing `docs/testing.md` to LF. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/resource_runtime.cpp,tests/unit/runtime_package_candidate_resident_mount_tests.cpp` | PASS | Focused C++ hygiene passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | Manifest fragments/composed output passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS | Agent surface consistency passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | AI integration/static guard drift passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Slice-closing gate passed after docs/manifest evidence update. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Pre-commit build gate passed after validation. |
