# Runtime Package Candidate Load v1 (2026-05-15)

**Plan ID:** `runtime-package-candidate-load-v1`
**Status:** Completed child slice under `production-completion-master-plan-v1` / `runtime-resource-v2`
**Goal:** Add a host-independent reviewed package candidate load contract that turns a selected `.geindex` discovery candidate into a typed `RuntimeAssetPackageLoadResult` without mounting, refreshing resident catalogs, streaming, or touching renderer/RHI resources.

## Context

`runtime-resource-v2` currently has resident mount sets, resident catalog cache refresh, package streaming resident mount/replace/unmount safe points, reviewed eviction planning, and deterministic `.geindex` discovery. The latest discovery slice intentionally avoids package reads and package loads. The next narrow disk/VFS step is a candidate-scoped load helper that callers can invoke after reviewing discovery results.

Existing `load_runtime_asset_package` already loads package index/payload content, but it is a low-level loader that can throw on unsafe descriptors or invalid package text. This slice adds a reviewed candidate wrapper with deterministic status and diagnostics so later host/editor/package-streaming code can select a discovered candidate, load it, and then pass the result into existing safe-point surfaces.

## Official Practice Check

- CMake: Context7 `/kitware/cmake` confirms target-scoped `add_executable`, `target_link_libraries`, and `add_test(NAME ... COMMAND <target>)` are the supported CMake/CTest pattern for adding unit-test executables. This slice follows existing repository target-scoped test wiring.
- C++/API: Follow `docs/cpp-style.md` public API naming/layout, C++23, RAII/value types, and no compatibility aliases.

## Constraints

- Keep the helper in `MK_runtime` and independent from editor, SDL3, renderer/RHI, upload/staging, native filesystem handles, background workers, and package-streaming mutation.
- Validate selected candidate paths again even when they originated from discovery; callers can construct candidates directly.
- Convert low-level loader failures and exceptions into typed candidate-load diagnostics.
- Do not mount packages, stage safe-point replacements, refresh `RuntimeResidentCatalogCacheV2`, mutate `RuntimeAssetPackageStore`, or infer `content_root`.
- Do not broaden ready claims for hot reload productization, renderer/RHI ownership, broad/background package streaming, upload/staging integration, arbitrary eviction, allocator/GPU budgets, or 2D/3D vertical slices.

## Tasks

- [x] Add RED tests in `tests/unit/runtime_package_candidate_load_tests.cpp` for successful candidate load, invalid candidates before reads, low-level package load failures, invalid package index text, and filesystem read exceptions.
- [x] Add the `MK_runtime_package_candidate_load_tests` CMake target using target-scoped CMake wiring.
- [x] Add public status/result/diagnostic types and `load_runtime_package_candidate_v2` to `engine/runtime/include/mirakana/runtime/resource_runtime.hpp`.
- [x] Implement candidate validation, load wrapping, diagnostics, and no-mutation behavior in `engine/runtime/src/resource_runtime.cpp`.
- [x] Update runtime/resource docs, roadmap/current capabilities, plan registry/master plan, and manifest fragments/composed manifest.
- [x] Run focused build/test/static checks, then full `validate.ps1` and `build.ps1`.

## Done When

- A reviewed `.geindex` candidate can be loaded into a typed result with record count and resident-byte estimate.
- Invalid candidate descriptors return `invalid_candidate` before reading the filesystem.
- Missing payloads, hash/dependency failures, invalid index text, and filesystem read exceptions return typed diagnostics without throwing.
- The helper leaves package stores, resident mount sets, resident catalog caches, package streaming state, renderer/RHI, and native handles untouched.
- `engine/agent/manifest.json` and current-truth docs accurately describe this as candidate load only, with disk/VFS mount execution and broader streaming still follow-up claims.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `cmake --build --preset dev --target MK_runtime_package_candidate_load_tests` | Passed | RED failed before implementation on the missing API/status types; GREEN target build passed after implementation and after the final no-unsafe-desc adjustment. |
| `ctest --preset dev --output-on-failure -R MK_runtime_package_candidate_load_tests` | Passed | Focused candidate load suite passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed | Public runtime header changed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed | Formatting/text checks passed after `tools/format.ps1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/resource_runtime.cpp,tests/unit/runtime_package_candidate_load_tests.cpp` | Passed | Focused C++ hygiene passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed | Manifest fragments and composed output passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed | Agent surface consistency passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | AI integration/static guard drift passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Slice-closing gate passed; host-gated Metal/Apple diagnostics remained diagnostic-only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Passed | Pre-commit build gate passed. |
