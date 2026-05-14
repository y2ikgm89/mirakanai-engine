# Runtime Package Streaming Resident Replace v1 (2026-05-15)

**Plan ID:** `runtime-package-streaming-resident-replace-v1`

## Goal

Add a reviewed package-streaming safe-point path that replaces one existing resident package mount through the already validated resident replacement commit, preserving mount order and catalog cache state on failure.

## Context

The `runtime-resource-v2` burn-down already has explicit resident package mount, catalog cache, resident mount execution, unmount/cache refresh, and `commit_runtime_resident_package_replace_v2` slices. The remaining follow-up is to make resident replacement usable from the selected package streaming execution surface, which is the operator/AI-facing safe-point path for generated package updates.

Official practice check:

- This slice does not touch CMake/vcpkg/SDL3/GPU/platform SDK behavior and adds no third-party dependency.
- The API remains value/result oriented, deterministic, and RAII-friendly, aligned with [C++ Core Guidelines R.1](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#r1-manage-resources-automatically-using-resource-handles-and-raii) resource-management guidance to keep ownership and mutation explicit through scoped values and resource handles.
- Project C++ style requires C++23, minimal public headers, no compatibility aliases, and designated aggregate initializers updated in declaration order when public aggregate fields change.

## Constraints

- Keep `MK_runtime` independent from renderer/RHI native handles, editor code, arbitrary shell execution, and background streaming claims.
- Do not add compatibility wrappers around the older single-package store replacement path.
- Do not claim broad hot reload, disk/VFS discovery, renderer/RHI ownership, GPU upload/staging, allocator enforcement, eviction, or background streaming.
- Preserve live mount set and cached catalog on invalid descriptor, missing/invalid mount id, package load failure, residency hint failure, budget failure, and candidate catalog failure.
- Update docs, manifest fragments, schemas/static checks, and skills only if the durable AI-operable contract changes.

## Implementation Checklist

- [x] Add failing tests in `tests/unit/runtime_package_streaming_resident_mount_tests.cpp` for selected resident package replacement commit success and failure preservation.
- [x] Add a package-streaming resident replace result path in `engine/runtime/include/mirakana/runtime/package_streaming.hpp`.
- [x] Implement the replace execution in `engine/runtime/src/package_streaming.cpp` by validating the descriptor, loaded package, residency hints, and then calling `commit_runtime_resident_package_replace_v2`.
- [x] Run focused build/test for `MK_runtime_package_streaming_resident_mount_tests`.
- [x] Reconcile current capabilities, roadmap/plan registry, manifest fragments + composed manifest, and static checks if the new execution path changes the AI-operable contract.
- [x] Run the slice-closing validation gate.

## Done When

- Selected package streaming can replace one existing resident mount at a safe point and refresh the resident catalog cache through the existing replacement commit.
- Replacement reports deterministic diagnostics and preserves previous state on every preflight or commit failure covered by tests.
- Ready claims stay narrow: no broad/background package streaming, renderer/RHI ownership, disk/VFS discovery, upload/staging integration, or hot-reload productization beyond this reviewed execution surface.
- Validation evidence is recorded below.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/prepare-worktree.ps1` | Passed | Linked worktree prepared; `external/vcpkg` linked from the main checkout. |
| `cmake --preset dev` | Passed | Direct host invocation was blocked by missing PATH compiler discovery; repository `tools/common.ps1` toolchain wrapper configured `dev` successfully. |
| `cmake --build --preset dev --target MK_runtime_package_streaming_resident_mount_tests` | Failed as expected before implementation | RED check proved the new safe-point API was missing. |
| `cmake --build --preset dev --target MK_runtime_package_streaming_resident_mount_tests` | Passed | Focused runtime package streaming resident mount/replace tests built after implementation and formatting cleanup. |
| `ctest --preset dev --output-on-failure -R MK_runtime_package_streaming_resident_mount_tests` | Passed | Focused resident package streaming replacement tests passed. |
| `cmake --build --preset dev --target MK_runtime_resource_resident_replace_tests MK_runtime_resource_resident_cache_tests MK_asset_identity_runtime_resource_tests` | Passed | Regression targets for resident replacement, catalog cache, and runtime resource identity built. |
| `ctest --preset dev --output-on-failure -R "^(MK_runtime_package_streaming_resident_mount_tests\|MK_runtime_resource_resident_replace_tests\|MK_runtime_resource_resident_cache_tests\|MK_asset_identity_runtime_resource_tests)$"` | Passed | Regression CTest set passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed | Public API boundary stayed within runtime package streaming contracts. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed | Ran after `tools/format.ps1` normalized the C++ changes. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/package_streaming.cpp,tests/unit/runtime_package_streaming_resident_mount_tests.cpp` | Passed | Focused clang-tidy passed after replacing the last `size() >= 1` assertion with `empty()`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed | Manifest composition and AI-operable package streaming contract checks passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed | Agent instruction/skill parity checks passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | Manifest, docs, source, and test needles for the resident replace execution surface passed. |
| `Invoke-ScriptAnalyzer -Path ... -Severity Error` | Blocked | `Invoke-ScriptAnalyzer` is not installed on this host; covered by repository agent/static guards and final validation instead. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Full slice gate passed; Metal/Apple lanes were reported as diagnostic-only / host-gated on this Windows host. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Passed | Commit-preflight build passed after full validation. |
